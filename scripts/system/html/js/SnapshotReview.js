"use strict";
//
//  SnapshotReview.js
//  scripts/system/html/js/
//
//  Created by Howard Stearns 8/22/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var paths = [], idCounter = 0, imageCount = 1;
function addImage(data) {
    if (!data.localPath) {
        return;
    }
    var div = document.createElement("DIV");
    var id = "p" + idCounter++;
    var img = document.createElement("IMG");
    div.id = id;
    img.id = id + "img";
    div.style.width = "100%";
    div.style.height = "" + 502 / imageCount + "px";
    div.style.display = "flex";
    div.style.justifyContent = "center";
    div.style.alignItems = "center";
    div.style.position = "relative";
    if (imageCount > 1) {
        img.setAttribute("class", "multiple");
    }
    img.src = data.localPath;
    div.appendChild(img);
    document.getElementById("snapshot-images").appendChild(div);
    var isGif = img.src.split('.').pop().toLowerCase() === "gif";
    div.appendChild(createShareOverlay(id, isGif));
    if (!isGif) {
        img.onload = function () {
            var shareBar = document.getElementById(id + "shareBar");
            shareBar.style.width = img.clientWidth;
            shareBar.style.display = "inline";

            document.getElementById(id).style.height = img.clientHeight;
        }
    }
    paths.push(data);
}
function createShareOverlay(parentID, isGif) {
    var shareOverlayContainer = document.createElement("DIV");
    shareOverlayContainer.id = parentID + "shareOverlayContainer";
    shareOverlayContainer.style.position = "absolute";
    shareOverlayContainer.style.top = "0px";
    shareOverlayContainer.style.left = "0px";
    shareOverlayContainer.style.display = "flex";
    shareOverlayContainer.style.alignItems = "flex-end";
    shareOverlayContainer.style.width = "100%";
    shareOverlayContainer.style.height = "100%";

    var shareBar = document.createElement("div");
    shareBar.id = parentID + "shareBar"
    shareBar.style.display = "none";
    shareBar.style.width = "100%";
    shareBar.style.height = "60px";
    shareBar.style.lineHeight = "60px";
    shareBar.style.clear = "both";
    shareBar.style.marginLeft = "auto";
    shareBar.style.marginRight = "auto";
    shareBar.innerHTML = isGif ? '<span class="gifLabel">GIF</span>' : "";
    var shareButtonID = parentID + "shareButton";
    shareBar.innerHTML += '<div class="shareButtonDiv">' +
            '<label class="shareButtonLabel" for="' + shareButtonID + '">SHARE</label>' +
            '<input type="button" class="shareButton" id="' + shareButtonID + '" onclick="selectImageToShare(' + parentID + ')" />' +
        '</div>'
    shareOverlayContainer.appendChild(shareBar);

    var shareOverlayBackground = document.createElement("div");
    shareOverlayBackground.id = parentID + "shareOverlayBackground";
    shareOverlayBackground.style.display = "none";
    shareOverlayBackground.style.position = "absolute";
    shareOverlayBackground.style.zIndex = "1";
    shareOverlayBackground.style.top = "0px";
    shareOverlayBackground.style.left = "0px";
    shareOverlayBackground.style.backgroundColor = "black";
    shareOverlayBackground.style.opacity = "0.5";
    shareOverlayBackground.style.width = "100%";
    shareOverlayBackground.style.height = "100%";
    shareOverlayContainer.appendChild(shareOverlayBackground);

    var shareOverlay = document.createElement("div");
    shareOverlay.id = parentID + "shareOverlay";
    shareOverlay.className = "shareOverlayDiv";
    shareOverlay.style.display = "none";
    shareOverlay.style.width = "100%";
    shareOverlay.style.height = "100%";
    shareOverlay.style.zIndex = "2";
    var shareWithEveryoneButtonID = parentID + "shareWithEveryoneButton";
    var inviteConnectionsCheckboxID = parentID + "inviteConnectionsCheckbox";
    shareOverlay.innerHTML = '<label class="shareOverlayLabel">SHARE</label>' +
        '<br/>' +
        '<div class="shareControls">' +
            '<div class="hifiShareControls">' +
                '<input type="button" class="shareWithEveryone" id="' + shareWithEveryoneButtonID + '" value="SHARE WITH EVERYONE" onclick="" /><br>' +
                '<input type="checkbox" class="inviteConnections" id="' + inviteConnectionsCheckboxID + '" checked="checked" />' +
                '<label class="shareButtonLabel" for="' + inviteConnectionsCheckboxID + '">Invite My Connections</label><br>' +
                '<input type="button" class="cancelShare" value="CANCEL" onclick="cancelSharing(' + parentID + ')" />' +
            '</div>' +
            '<div class="externalShareControls">' +
                '<iframe src="https://www.facebook.com/plugins/share_button.php?href=http%3A%2F%2Fhighfidelity.io&layout=button_count&size=small&mobile_iframe=false&width=84&height=20&appId" width="84" height="20" style="border:none;overflow:hidden" scrolling="no" frameborder="0" allowTransparency="true"></iframe>' +
                '<a class="twitter-share-button" href="https://twitter.com/intent/tweet">Tweet</a>' +
            '</div>' +
        '</div>';
    shareOverlayContainer.appendChild(shareOverlay);
    twttr.widgets.load(shareOverlay);

    return shareOverlayContainer;
}
function selectImageToShare(selectedID) {
    selectedID = selectedID.id; // Why is this necessary?
    var shareOverlayContainer = document.getElementById(selectedID + "shareOverlayContainer");
    var shareBar = document.getElementById(selectedID + "shareBar");
    var shareOverlayBackground = document.getElementById(selectedID + "shareOverlayBackground");
    var shareOverlay = document.getElementById(selectedID + "shareOverlay");

    shareOverlay.style.outline = "4px solid #00b4ef";
    shareOverlay.style.outlineOffset = "-4px";

    shareBar.style.display = "none";

    shareOverlayBackground.style.display = "inline";
    shareOverlay.style.display = "inline";
}
function cancelSharing(selectedID) {
    selectedID = selectedID.id; // Why is this necessary?
    var shareOverlayContainer = document.getElementById(selectedID + "shareOverlayContainer");
    var shareBar = document.getElementById(selectedID + "shareBar");
    var shareOverlayBackground = document.getElementById(selectedID + "shareOverlayBackground");
    var shareOverlay = document.getElementById(selectedID + "shareOverlay");

    shareOverlay.style.outline = "none";

    shareBar.style.display = "inline";

    shareOverlayBackground.style.display = "none";
    shareOverlay.style.display = "none";
}

function handleCaptureSetting(setting) {
    var stillAndGif = document.getElementById('stillAndGif');
    var stillOnly = document.getElementById('stillOnly');
    stillAndGif.checked = setting;
    stillOnly.checked = !setting;

    stillAndGif.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "captureStillAndGif"
        }));
    }
    stillOnly.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "captureStillOnly"
        }));
    }

}
window.onload = function () {
    // TESTING FUNCTIONS START
    // Uncomment and modify the lines below to test SnapshotReview in a browser.
    //imageCount = 2;
    //addImage({ localPath: 'C:/Users/Zach Fox/Desktop/hifi-snap-by-zfox-on-2017-04-20_14-59-12.gif' });
    //addImage({ localPath: 'C:/Users/Zach Fox/Desktop/hifi-snap-by-zfox-on-2017-04-20_14-59-12.jpg' });
    //addImage({ localPath: 'http://lorempixel.com/553/255' });
    //addImage({localPath: 'c:/Users/howar/OneDrive/Pictures/hifi-snap-by--on-2016-07-27_12-58-43.jpg'});
    // TESTING FUNCTIONS END

    openEventBridge(function () {
        // Set up a handler for receiving the data, and tell the .js we are ready to receive it.
        EventBridge.scriptEventReceived.connect(function (message) {

            message = JSON.parse(message);

            if (message.type !== "snapshot") {
                return;
            }
            
            switch (message.action) {
                case 'addImages':
                    // The last element of the message contents list contains a bunch of options,
                    // including whether or not we can share stuff
                    // The other elements of the list contain image paths.
                    var messageOptions = message.options;

                    if (messageOptions.containsGif) {
                        if (messageOptions.processingGif) {
                            imageCount = message.data.length + 1; // "+1" for the GIF that'll finish processing soon
                            message.data.unshift({ localPath: messageOptions.loadingGifPath });
                            message.data.forEach(addImage);
                        } else {
                            var gifPath = message.data[0].localPath;
                            var p0img = document.getElementById('p0img');
                            p0img.src = gifPath;

                            p0img.onload = function () {
                                var shareBar = document.getElementById("p0shareBar");
                                shareBar.style.width = p0img.clientWidth;
                                shareBar.style.display = "inline";
                                document.getElementById('p0').style.height = p0img.clientHeight;
                            }

                            paths[0].localPath = gifPath;
                        }
                    } else {
                        imageCount = message.data.length;
                        message.data.forEach(addImage);
                    }
                    break;
                case 'captureSettings':
                    handleCaptureSetting(message.setting);
                    break;
                default:
                    print("Unknown message action received in SnapshotReview.js.");
                    break;
            }

        });

        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "ready"
        }));
    });

};
// beware of bug: Cannot send objects at top level. (Nested in arrays is fine.)
function shareSelected() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: paths
    }));
}
function doNotShare() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: []
    }));
}
function snapshotSettings() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "openSettings"
    }));
}
function takeSnapshot() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "takeSnapshot"
    }));
}
