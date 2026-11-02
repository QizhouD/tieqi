#pragma once

#include "app/Pointer.h"
#include "app/Viewport.h"

#include <cmath>

namespace tq {

// Zoom factor per wheel notch, and how far a right drag moves before it pans instead of ordering.
inline constexpr float kWheelZoomStep = 1.25f;
inline constexpr float kPanThresholdPx = 6.f;

// Wheel zoom around the pointer, middle-drag or right-drag panning, and Home to reset. Right-drag pans only while
// zoomed in, so a right click still orders.
class CameraControl {
  public:
    struct Result {
        // The sample belongs to the camera and must not reach the orders.
        bool consumed = false;
        // A right press just turned into a pan; the pending press must be dropped from the orders.
        bool startedPan = false;
    };

    Result handle(const PointerSample &sample, float mapW, float mapH) {
        Result result;
        if (sample.type == PointerSample::Type::Wheel) {
            if (insideField(sample.x, sample.y) && sample.wheel != 0.f) {
                zoom_ = zoomAround(zoom_, std::pow(kWheelZoomStep, sample.wheel), sample.x, sample.y, mapW, mapH);
                result.consumed = true;
            }
            return result;
        }
        if (sample.device != PointerSample::Device::Mouse) {
            return result;
        }
        if (sample.type == PointerSample::Type::Down) {
            if (!insideField(sample.x, sample.y) ||
                (sample.button != kPointerMiddle && !(sample.button == kPointerRight && zoom_.level > kMinZoom))) {
                return result;
            }
            pressing_ = true;
            button_ = sample.button;
            startX_ = lastX_ = sample.x;
            startY_ = lastY_ = sample.y;
            panning_ = sample.button == kPointerMiddle;
            result.consumed = panning_;
            return result;
        }
        if (!pressing_) {
            return result;
        }
        if (sample.type == PointerSample::Type::Move) {
            if (!panning_ && std::hypot(sample.x - startX_, sample.y - startY_) > kPanThresholdPx) {
                panning_ = true;
                result.startedPan = true;
            }
            if (panning_) {
                zoom_ = panBy(zoom_, sample.x - lastX_, sample.y - lastY_, mapW, mapH);
                lastX_ = sample.x;
                lastY_ = sample.y;
                result.consumed = true;
            }
            return result;
        }
        if (sample.button == button_) {
            result.consumed = panning_;
            pressing_ = false;
            panning_ = false;
        }
        return result;
    }

    void reset() { *this = CameraControl{}; }
    const Zoom &zoom() const { return zoom_; }
    void setZoom(const Zoom &zoom) { zoom_ = zoom; }
    bool panning() const { return panning_; }

  private:
    static bool insideField(float x, float y) {
        const ScreenRect field = battlefieldRect();
        return x >= field.x && x < field.x + field.w && y >= field.y && y < field.y + field.h;
    }

    Zoom zoom_;
    bool pressing_ = false;
    bool panning_ = false;
    int button_ = kPointerMiddle;
    float startX_ = 0.f;
    float startY_ = 0.f;
    float lastX_ = 0.f;
    float lastY_ = 0.f;
};

} // namespace tq
