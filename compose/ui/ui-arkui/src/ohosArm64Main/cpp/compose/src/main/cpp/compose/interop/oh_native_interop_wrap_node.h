#ifndef OH_NATIVE_INTEROP_WRAP_VIEW_H
#define OH_NATIVE_INTEROP_WRAP_VIEW_H

#include <functional>
#include <memory>
#include <napi/native_api.h>
#include <arkui/native_type.h>
#include "../render_node/oh_base_render_node.h"

// Forward declaration
namespace OH {
    struct TouchEvent;
}

namespace OH {
    class InteropWrapView {
    public:
       // using SizeChangeCallback = std::function<void(float width, float height)>;
        struct Rect {
            float x;
            float y;
            float width;
            float height;
        };
        
        InteropWrapView();
        ~InteropWrapView();

        void setUserInteraction(bool interactionEnable);
        OH::BaseRenderNode* getMixedRendNode();
        void Initialize(napi_env env, napi_ref createArkUIView, ArkUI_NodeHandle customNodeHandle);
        ArkUI_NodeHandle CreateMixedNode(const char* name, napi_value parameter);
        void onTouchEvent(const struct TouchEvent& event);
        napi_value getJsArkUIView();

    private:
        static void maybeThrow(const int32_t status) {
            if (status != ARKUI_ERROR_CODE_NO_ERROR) {
                LOGE("OHRenderNode operation failed with status: %{public}d", status);
                throw std::runtime_error("OHRenderNode operation failed");
            }
        }
        napi_value stringToNAPIValue(napi_env env, const char* str);
        napi_value callArkUIVIewMethod(napi_env env, napi_value object, const char* method_name,
                size_t argc, napi_value* argv);
        napi_value getArkUIViewProperty(napi_env env, napi_value object, const char* property_name);
        void releaseMixedViewRef();
        void setMixedViewRef(napi_value jsView);
        Rect lastFrame_;
        std::unique_ptr<OH::BaseRenderNode> renderNode_ = std::make_unique<OH::BaseRenderNode>();
        std::unique_ptr<OH::BaseRenderNode> m_mixedRenderNode = nullptr;
        napi_env m_env = nullptr;
        napi_ref m_createArkUIView = nullptr;
        napi_ref m_mixedViewRef = nullptr;
        ArkUI_NodeHandle m_customNodeHandle = nullptr;
        ArkUI_NodeHandle m_mixedHandle = nullptr;
        bool m_shouldHandlerTouch = true;
    };
} // namespace OH

#endif // OH_NATIVE_INTEROP_WRAP_VIEW_H