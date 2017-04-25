//
//  Created by Dante Ruiz 2017/04/16
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InputRecorder.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QDateTime>
#include <QByteArray>
#include <QStandardPaths>
#include <PathUtils.h>

#include <BuildInfo.h>
#include <GLMHelpers.h>
 
QString SAVE_DIRECTORY = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + BuildInfo::INTERFACE_NAME +
    "/interface" + "/hifi-input-recordings/";
QString FILE_PREFIX_NAME = "input-recording-";
QString COMPRESS_EXTENSION = ".tar.gz";
namespace controller {
    
    QJsonObject poseToJsonObject(const Pose pose) {
        QJsonObject newPose;
        
        QJsonArray translation;
        translation.append(pose.translation.x);
        translation.append(pose.translation.y);
        translation.append(pose.translation.z);
        
        QJsonArray rotation;
        rotation.append(pose.rotation.x);
        rotation.append(pose.rotation.y);
        rotation.append(pose.rotation.z);
        rotation.append(pose.rotation.w);

        QJsonArray velocity;
        velocity.append(pose.velocity.x);
        velocity.append(pose.velocity.y);
        velocity.append(pose.velocity.z);

        QJsonArray angularVelocity;
        angularVelocity.append(pose.angularVelocity.x);
        angularVelocity.append(pose.angularVelocity.y);
        angularVelocity.append(pose.angularVelocity.z);

        newPose["translation"] = translation;
        newPose["rotation"] = rotation;
        newPose["velocity"] = velocity;
        newPose["angularVelocity"] = angularVelocity;
        newPose["valid"] = pose.valid;

        return newPose;
    }

    Pose jsonObjectToPose(const QJsonObject object) {
        Pose pose;
        QJsonArray translation = object["translation"].toArray();
        QJsonArray rotation = object["rotation"].toArray();
        QJsonArray velocity = object["velocity"].toArray();
        QJsonArray angularVelocity = object["angularVelocity"].toArray();

        pose.valid = object["valid"].toBool();
        
        pose.translation.x = translation[0].toDouble();
        pose.translation.y = translation[1].toDouble();
        pose.translation.z = translation[2].toDouble();

        pose.rotation.x = rotation[0].toDouble();
        pose.rotation.y = rotation[1].toDouble();
        pose.rotation.z = rotation[2].toDouble();
        pose.rotation.w = rotation[3].toDouble();

        pose.velocity.x = velocity[0].toDouble();
        pose.velocity.y = velocity[1].toDouble();
        pose.velocity.z = velocity[2].toDouble();

        pose.angularVelocity.x = angularVelocity[0].toDouble();
        pose.angularVelocity.y = angularVelocity[1].toDouble();
        pose.angularVelocity.z = angularVelocity[2].toDouble();

        return pose;
    }
        

    void exportToFile(QJsonObject& object) {
        if (!QDir(SAVE_DIRECTORY).exists()) {
            QDir().mkdir(SAVE_DIRECTORY);
        }
        
        QString timeStamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        timeStamp.replace(":", "-");
        QString fileName = SAVE_DIRECTORY + FILE_PREFIX_NAME + timeStamp + COMPRESS_EXTENSION;
        qDebug() << fileName;
        QFile saveFile (fileName);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            qWarning() << "could not open file: " << fileName;
            return;
        }
        QJsonDocument saveData(object);
        QByteArray compressedData = qCompress(saveData.toJson(QJsonDocument::Compact));
        saveFile.write(compressedData);
    }

    QJsonObject openFile(const QString& file, bool& status) {
        QJsonObject object;
        QFile openFile(file);
        if (!openFile.open(QIODevice::ReadOnly)) {
            qWarning() << "could not open file: " << file;
            status = false;
            return object;
        }
        QByteArray compressedData = qUncompress(openFile.readAll());
        QJsonDocument jsonDoc = QJsonDocument::fromJson(compressedData);
        object = jsonDoc.object();
        status = true;
        return object;
    }
 
    InputRecorder::InputRecorder() {}

    InputRecorder::~InputRecorder() {}

    InputRecorder* InputRecorder::getInstance() {
        static InputRecorder inputRecorder;
        return &inputRecorder;
    }

    void InputRecorder::startRecording() {
        _recording = true;
        _playback = false;
        _framesRecorded = 0;
        _poseStateList.clear();
        _actionStateList.clear();
    }

    void InputRecorder::saveRecording() {
        QJsonObject data;
        data["frameCount"] = _framesRecorded;

        QJsonArray actionArrayList;
        QJsonArray poseArrayList;
        for(const ActionStates actionState: _actionStateList) {
            QJsonArray actionArray;
            for (const float value: actionState) {
                actionArray.append(value);
            }
            actionArrayList.append(actionArray);
        }

        for (const PoseStates poseState: _poseStateList) {
            QJsonArray poseArray;
            for (const Pose pose: poseState) {
                poseArray.append(poseToJsonObject(pose));
            }
            poseArrayList.append(poseArray);
        }

        data["actionList"] = actionArrayList;
        data["poseList"] = poseArrayList;
        exportToFile(data);
    }

    void InputRecorder::loadRecording(const QString& path) {
        _recording = false;
        _playback = false;
        _loading = true;
        _playCount = 0;
        resetFrame();
        _poseStateList.clear();
        _actionStateList.clear();
        QString filePath = path;
        filePath.remove(0,8);
        QFileInfo info(filePath);
        QString extension = info.suffix();
         if (extension != "gz") {
             qWarning() << "can not load file with exentsion of " << extension;
             return;
         }
         bool success = false;
         QJsonObject data = openFile(info.absoluteFilePath(), success);
         if (success) {
             _framesRecorded = data["frameCount"].toInt();
             QJsonArray actionArrayList = data["actionList"].toArray();
             QJsonArray poseArrayList = data["poseList"].toArray();
             
             for (int actionIndex = 0; actionIndex < actionArrayList.size(); actionIndex++) {
                 QJsonArray actionState = actionArrayList[actionIndex].toArray();
                 for (int index = 0; index < actionState.size(); index++) {
                     _currentFrameActions[index] = actionState[index].toInt();
                 }
                 _actionStateList.push_back(_currentFrameActions);
                 _currentFrameActions = ActionStates(toInt(Action::NUM_ACTIONS));
             }
             
             for (int poseIndex = 0; poseIndex < poseArrayList.size(); poseIndex++) {
                 QJsonArray poseState = poseArrayList[poseIndex].toArray();
                 for (int index = 0; index < poseState.size(); index++) {
                     _currentFramePoses[index] = jsonObjectToPose(poseState[index].toObject());
                 }
                 _poseStateList.push_back(_currentFramePoses);
                 _currentFramePoses = PoseStates(toInt(Action::NUM_ACTIONS));
             }
         }

         _loading = false;
    }

    void InputRecorder::stopRecording() {
        _recording = false;
    }

    void InputRecorder::startPlayback() {
        _playback = true;
        _recording = false;
        _playCount = 0;
    }

    void InputRecorder::stopPlayback() {
        _playback = false;
        _playCount = 0;
    }

    void InputRecorder::setActionState(controller::Action action, float value) {
        if (_recording) {
            _currentFrameActions[toInt(action)] += value;
        }
    }

    void InputRecorder::setActionState(controller::Action action, const controller::Pose pose) {
        if (_recording) {
            _currentFramePoses[toInt(action)] = pose;
        }
    }

    void InputRecorder::resetFrame() {
        if (_recording) {
            for(auto& channel : _currentFramePoses) {
                channel = Pose();
            }
            
            for(auto& channel : _currentFrameActions) {
                channel = 0.0f;
            }
        }
    }
    
    float InputRecorder::getActionState(controller::Action action) {
        if (_actionStateList.size() > 0 ) {
            return _actionStateList[_playCount][toInt(action)];
        }

        return 0.0f;
    }

    controller::Pose InputRecorder::getPoseState(controller::Action action) {
        if (_poseStateList.size() > 0) {
            return _poseStateList[_playCount][toInt(action)];
        }

        return Pose();
    }

    void InputRecorder::frameTick() {
        if (_recording) {
            _framesRecorded++;
            _poseStateList.push_back(_currentFramePoses);
            _actionStateList.push_back(_currentFrameActions);
        }

        if (_playback) {
            _playCount++;
            if (_playCount == _framesRecorded) {
                _playCount = 0;
            }
        }
    }
}
