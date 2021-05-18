/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrClampedGradientEffect.fp; do not modify.
 **************************************************************************************************/
#include "GrClampedGradientEffect.h"

#include "src/core/SkUtils.h"
#include "src/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
class GrGLSLClampedGradientEffect : public GrGLSLFragmentProcessor {
public:
    GrGLSLClampedGradientEffect() {}
    void emitCode(EmitArgs& args) override {
        GrGLSLFPFragmentBuilder* fragBuilder = args.fFragBuilder;
        const GrClampedGradientEffect& _outer = args.fFp.cast<GrClampedGradientEffect>();
        (void)_outer;
        auto leftBorderColor = _outer.leftBorderColor;
        (void)leftBorderColor;
        auto rightBorderColor = _outer.rightBorderColor;
        (void)rightBorderColor;
        auto makePremul = _outer.makePremul;
        (void)makePremul;
        auto colorsAreOpaque = _outer.colorsAreOpaque;
        (void)colorsAreOpaque;
        auto layoutPreservesOpacity = _outer.layoutPreservesOpacity;
        (void)layoutPreservesOpacity;
        leftBorderColorVar = args.fUniformHandler->addUniform(
                &_outer, kFragment_GrShaderFlag, kHalf4_GrSLType, "leftBorderColor");
        rightBorderColorVar = args.fUniformHandler->addUniform(
                &_outer, kFragment_GrShaderFlag, kHalf4_GrSLType, "rightBorderColor");
        SkString _sample0 = this->invokeChild(1, args);
        fragBuilder->codeAppendf(
                R"SkSL(half4 t = %s;
half4 outColor;
if (!%s && t.y < 0.0) {
    outColor = half4(0.0);
} else if (t.x < 0.0) {
    outColor = %s;
} else if (t.x > 1.0) {
    outColor = %s;
} else {)SkSL",
                _sample0.c_str(),
                (_outer.layoutPreservesOpacity ? "true" : "false"),
                args.fUniformHandler->getUniformCStr(leftBorderColorVar),
                args.fUniformHandler->getUniformCStr(rightBorderColorVar));
        SkString _coords1("float2(half2(t.x, 0.0))");
        SkString _sample1 = this->invokeChild(0, args, _coords1.c_str());
        fragBuilder->codeAppendf(
                R"SkSL(
    outColor = %s;
}
@if (%s) {
    outColor.xyz *= outColor.w;
}
return outColor;
)SkSL",
                _sample1.c_str(),
                (_outer.makePremul ? "true" : "false"));
    }

private:
    void onSetData(const GrGLSLProgramDataManager& pdman,
                   const GrFragmentProcessor& _proc) override {
        const GrClampedGradientEffect& _outer = _proc.cast<GrClampedGradientEffect>();
        {
            pdman.set4fv(leftBorderColorVar, 1, (_outer.leftBorderColor).vec());
            pdman.set4fv(rightBorderColorVar, 1, (_outer.rightBorderColor).vec());
        }
    }
    UniformHandle leftBorderColorVar;
    UniformHandle rightBorderColorVar;
};
std::unique_ptr<GrGLSLFragmentProcessor> GrClampedGradientEffect::onMakeProgramImpl() const {
    return std::make_unique<GrGLSLClampedGradientEffect>();
}
void GrClampedGradientEffect::onGetGLSLProcessorKey(const GrShaderCaps& caps,
                                                    GrProcessorKeyBuilder* b) const {
    b->addBool(makePremul, "makePremul");
    b->addBool(layoutPreservesOpacity, "layoutPreservesOpacity");
}
bool GrClampedGradientEffect::onIsEqual(const GrFragmentProcessor& other) const {
    const GrClampedGradientEffect& that = other.cast<GrClampedGradientEffect>();
    (void)that;
    if (leftBorderColor != that.leftBorderColor) return false;
    if (rightBorderColor != that.rightBorderColor) return false;
    if (makePremul != that.makePremul) return false;
    if (colorsAreOpaque != that.colorsAreOpaque) return false;
    if (layoutPreservesOpacity != that.layoutPreservesOpacity) return false;
    return true;
}
GrClampedGradientEffect::GrClampedGradientEffect(const GrClampedGradientEffect& src)
        : INHERITED(kGrClampedGradientEffect_ClassID, src.optimizationFlags())
        , leftBorderColor(src.leftBorderColor)
        , rightBorderColor(src.rightBorderColor)
        , makePremul(src.makePremul)
        , colorsAreOpaque(src.colorsAreOpaque)
        , layoutPreservesOpacity(src.layoutPreservesOpacity) {
    this->cloneAndRegisterAllChildProcessors(src);
}
std::unique_ptr<GrFragmentProcessor> GrClampedGradientEffect::clone() const {
    return std::make_unique<GrClampedGradientEffect>(*this);
}
#if GR_TEST_UTILS
SkString GrClampedGradientEffect::onDumpInfo() const {
    return SkStringPrintf(
            "(leftBorderColor=half4(%f, %f, %f, %f), rightBorderColor=half4(%f, %f, %f, %f), "
            "makePremul=%s, colorsAreOpaque=%s, layoutPreservesOpacity=%s)",
            leftBorderColor.fR,
            leftBorderColor.fG,
            leftBorderColor.fB,
            leftBorderColor.fA,
            rightBorderColor.fR,
            rightBorderColor.fG,
            rightBorderColor.fB,
            rightBorderColor.fA,
            (makePremul ? "true" : "false"),
            (colorsAreOpaque ? "true" : "false"),
            (layoutPreservesOpacity ? "true" : "false"));
}
#endif
