//
//  BloomEffect.cpp
//  render-utils/src/
//
//  Created by Olivier Prat on 09/25/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "BloomEffect.h"

#include "gpu/Context.h"
#include "gpu/StandardShaderLib.h"

#include <render/BlurTask.h>

#include "BloomThreshold_frag.h"

#define BLOOM_BLUR_LEVEL_COUNT  3

ThresholdAndDownsampleJob::ThresholdAndDownsampleJob() {

}

void ThresholdAndDownsampleJob::configure(const Config& config) {
    _threshold = config.threshold;
}

void ThresholdAndDownsampleJob::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto frameTransform = inputs.get0();
    const auto inputFrameBuffer = inputs.get1();

    assert(inputFrameBuffer->hasColor());

    auto inputColor = inputFrameBuffer->getRenderBuffer(0);
    auto sourceViewport = args->_viewport;
    auto fullSize = glm::ivec2(inputColor->getDimensions());
    auto halfSize = fullSize / 2;
    auto halfViewport = args->_viewport >> 1;

    if (!_downsampledBuffer || _downsampledBuffer->getSize().x != halfSize.x || _downsampledBuffer->getSize().y != halfSize.y) {
        auto colorTexture = gpu::TexturePointer(gpu::Texture::createRenderBuffer(inputColor->getTexelFormat(), halfSize.x, halfSize.y, 
                                                gpu::Texture::SINGLE_MIP, gpu::Sampler(gpu::Sampler::FILTER_MIN_POINT_MAG_LINEAR)));

        _downsampledBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("BloomBlur0"));
        _downsampledBuffer->setRenderBuffer(0, colorTexture);
    }

    static const int COLOR_MAP_SLOT = 0;
    static const int THRESHOLD_SLOT = 1;

    if (!_pipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(BloomThreshold_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("colorMap", COLOR_MAP_SLOT));
        slotBindings.insert(gpu::Shader::Binding("threshold", THRESHOLD_SLOT));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        _pipeline = gpu::Pipeline::create(program, state);
    }

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setViewportTransform(halfViewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(inputFrameBuffer->getSize(), args->_viewport));
        batch.setPipeline(_pipeline);

        batch.setFramebuffer(_downsampledBuffer);
        batch.setResourceTexture(COLOR_MAP_SLOT, inputColor);
        batch._glUniform1f(THRESHOLD_SLOT, _threshold);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setViewportTransform(args->_viewport);
        batch.setResourceTexture(COLOR_MAP_SLOT, nullptr);
        batch.setFramebuffer(nullptr);
    });

    outputs = _downsampledBuffer;
}

DebugBloom::DebugBloom() {
}

DebugBloom::~DebugBloom() {
}

void DebugBloom::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    const auto frameBuffer = inputs.get0();
    const auto framebufferSize = frameBuffer->getSize();
    const auto level0FB = inputs.get1();
    const auto level1FB = inputs.get2();
    const auto level2FB = inputs.get3();
    const gpu::TexturePointer levelTextures[BLOOM_BLUR_LEVEL_COUNT] = {
        level0FB->getRenderBuffer(0),
        level1FB->getRenderBuffer(0),
        level2FB->getRenderBuffer(0)
    };

    if (!_pipeline) {
        auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
        auto ps = gpu::StandardShaderLib::getDrawTextureOpaquePS();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false));
        _pipeline = gpu::Pipeline::create(program, state);
    }

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setFramebuffer(frameBuffer);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setPipeline(_pipeline);

        Transform modelTransform = gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport / 2);
        modelTransform.postTranslate(glm::vec3(-1.0f, 1.0f, 0.0f));
        batch.setModelTransform(modelTransform);
        batch.setResourceTexture(0, levelTextures[0]);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        modelTransform.postTranslate(glm::vec3(2.0f, 0.0f, 0.0f));
        batch.setModelTransform(modelTransform);
        batch.setResourceTexture(0, levelTextures[1]);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        modelTransform.postTranslate(glm::vec3(-2.0f, -2.0f, 0.0f));
        batch.setModelTransform(modelTransform);
        batch.setResourceTexture(0, levelTextures[2]);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}

void BloomConfig::setIntensity(float value) {
}

void BloomConfig::setSize(float value) {
    for (auto i = 0; i < BLOOM_BLUR_LEVEL_COUNT; i++) {
        auto& blurJob = static_cast<render::Task::TaskConcept*>(_task)->_jobs[i+1];
        auto& gaussianBlur = blurJob.edit<render::BlurGaussian>();
        auto gaussianBlurParams = gaussianBlur.getParameters();
        gaussianBlurParams->setFilterGaussianTaps((BLUR_MAX_NUM_TAPS - 1) / 2, value*7.0f);
    }
}

Bloom::Bloom() {

}

void Bloom::configure(const Config& config) {
    std::string blurName{ "BloomBlurN" };

    for (auto i = 0; i < BLOOM_BLUR_LEVEL_COUNT; i++) {
        blurName.back() = '0' + i;
        auto blurConfig = config.getConfig<render::BlurGaussian>(blurName);
        blurConfig->setProperty("filterScale", 3.0f);
    }
}

void Bloom::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs) {
    const auto bloomInputBuffer = task.addJob<ThresholdAndDownsampleJob>("BloomThreshold", inputs);

    // Level 0 blur
    const auto blurFB0 = task.addJob<render::BlurGaussian>("BloomBlur0", bloomInputBuffer);
    const auto blurFB1 = task.addJob<render::BlurGaussian>("BloomBlur1", blurFB0, true);
    const auto blurFB2 = task.addJob<render::BlurGaussian>("BloomBlur2", blurFB1, true);

    const auto& input = inputs.get<Inputs>();
    const auto& frameBuffer = input[1];
    const auto debugInput = DebugBloom::Inputs(frameBuffer, blurFB0, blurFB1, blurFB2).asVarying();
    task.addJob<DebugBloom>("DebugBloom", debugInput);
}
