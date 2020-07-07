/*
 * Copyright 2019 Google LLC.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkCanvas.h"
#include "samplecode/Sample.h"
#include "src/core/SkPathPriv.h"
#include "tools/ToolUtils.h"

#if SK_SUPPORT_GPU

#include "include/gpu/GrContext.h"
#include "src/gpu/GrClip.h"
#include "src/gpu/GrContextPriv.h"
#include "src/gpu/GrMemoryPool.h"
#include "src/gpu/GrRenderTargetContext.h"
#include "src/gpu/GrRenderTargetContextPriv.h"
#include "src/gpu/tessellate/GrTessellatePathOp.h"

// This sample enables wireframe and visualizes the triangulation generated by
// GrTessellateWedgeShader.
class TessellatedWedge : public Sample {
public:
    TessellatedWedge() {
#if 0
        fPath.moveTo(1, 0);
        int numSides = 32 * 3;
        for (int i = 1; i < numSides; ++i) {
            float theta = 2*3.1415926535897932384626433832785 * i / numSides;
            fPath.lineTo(std::cos(theta), std::sin(theta));
        }
        fPath.transform(SkMatrix::Scale(200, 200));
        fPath.transform(SkMatrix::Translate(300, 300));
#else
        fPath.moveTo(100, 200);
        fPath.cubicTo(100, 100, 400, 100, 400, 200);
        fPath.lineTo(250, 500);
#endif
    }

private:
    void onDrawContent(SkCanvas*) override;
    Sample::Click* onFindClickHandler(SkScalar x, SkScalar y, skui::ModifierKey) override;
    bool onClick(Sample::Click*) override;
    bool onChar(SkUnichar) override;

    SkString name() override { return SkString("TessellatedWedge"); }

    SkMatrix fLastViewMatrix = SkMatrix::I();
    SkPath fPath;
    GrTessellationPathRenderer::OpFlags fOpFlags = GrTessellationPathRenderer::OpFlags::kWireframe;

    class Click;
};

void TessellatedWedge::onDrawContent(SkCanvas* canvas) {
    canvas->clear(SK_ColorBLACK);

    GrContext* ctx = canvas->getGrContext();
    GrRenderTargetContext* rtc = canvas->internal_private_accessTopLayerRenderTargetContext();

    SkString error;
    if (!rtc || !ctx) {
        error = "GPU Only.";
    } else if (!ctx->priv().caps()->drawInstancedSupport()) {
        error = "Instanced rendering not supported.";
    } else if (1 == rtc->numSamples() && !ctx->priv().caps()->mixedSamplesSupport()) {
        error = "MSAA/mixed samples only.";
    }
    if (!error.isEmpty()) {
        SkFont font(nullptr, 20);
        SkPaint captionPaint;
        captionPaint.setColor(SK_ColorWHITE);
        canvas->drawString(error.c_str(), 10, 30, font, captionPaint);
        return;
    }

    GrPaint paint;
    paint.setColor4f({1,0,1,1});

    GrAAType aa;
    if (rtc->numSamples() > 1) {
        aa = GrAAType::kMSAA;
    } else if (rtc->asRenderTargetProxy()->canUseMixedSamples(*ctx->priv().caps())) {
        aa = GrAAType::kCoverage;
    } else {
        aa = GrAAType::kNone;
    }

    GrOpMemoryPool* pool = ctx->priv().opMemoryPool();
    rtc->priv().testingOnly_addDrawOp(pool->allocate<GrTessellatePathOp>(
            canvas->getTotalMatrix(), fPath, std::move(paint), aa, fOpFlags));

    // Draw the path points.
    SkPaint pointsPaint;
    pointsPaint.setColor(SK_ColorBLUE);
    pointsPaint.setStrokeWidth(8);
    SkPath devPath = fPath;
    devPath.transform(canvas->getTotalMatrix());
    {
        SkAutoCanvasRestore acr(canvas, true);
        canvas->setMatrix(SkMatrix::I());
        canvas->drawPoints(SkCanvas::kPoints_PointMode, devPath.countPoints(),
                           SkPathPriv::PointData(devPath), pointsPaint);
    }

    fLastViewMatrix = canvas->getTotalMatrix();
}

class TessellatedWedge::Click : public Sample::Click {
public:
    Click(int ptIdx) : fPtIdx(ptIdx) {}

    void doClick(SkPath* path) {
        if (fPtIdx >= 0) {
            SkPoint pt = path->getPoint(fPtIdx);
            SkPathPriv::UpdatePathPoint(path, fPtIdx, pt + fCurr - fPrev);
        } else {
            path->transform(
                    SkMatrix::Translate(fCurr.x() - fPrev.x(), fCurr.y() - fPrev.y()), path);
        }
    }

private:
    int fPtIdx;
};

Sample::Click* TessellatedWedge::onFindClickHandler(SkScalar x, SkScalar y, skui::ModifierKey) {
    const SkPoint* pts = SkPathPriv::PointData(fPath);
    float fuzz = 20 / fLastViewMatrix.getMaxScale();
    for (int i = 0; i < fPath.countPoints(); ++i) {
        SkPoint screenPoint = pts[i];
        if (fabs(x - screenPoint.x()) < fuzz && fabsf(y - screenPoint.y()) < fuzz) {
            return new Click(i);
        }
    }
    return new Click(-1);
}

bool TessellatedWedge::onClick(Sample::Click* click) {
    Click* myClick = (Click*)click;
    myClick->doClick(&fPath);
    return true;
}

bool TessellatedWedge::onChar(SkUnichar unichar) {
    switch (unichar) {
        case 'w':
            fOpFlags = (GrTessellationPathRenderer::OpFlags)(
                    (int)fOpFlags ^ (int)GrTessellationPathRenderer::OpFlags::kWireframe);
            return true;
        case 'D': {
            fPath.dump();
            return true;
        }
    }
    return false;
}

Sample* MakeTessellatedWedgeSample() { return new TessellatedWedge; }
static SampleRegistry gTessellatedWedgeSample(MakeTessellatedWedgeSample);

#endif  // SK_SUPPORT_GPU
