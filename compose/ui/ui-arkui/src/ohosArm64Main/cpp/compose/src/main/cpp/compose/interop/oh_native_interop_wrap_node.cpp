

#include "oh_native_interop_wrap_node.h"
#include "oh_interop_touch_event_handler.h"
#include <iostream>
#include "../xcomponent_log.h"
#include "../native_node_api.h"
#include <arkui/native_node_napi.h>
#include <arkui/native_render.h>
#include <cmath>

namespace OH {
    //static void* kFrameObserverContext = &kFrameObserverContext;

    InteropWrapView::InteropWrapView() 
        : lastFrame_({0, 0, 0, 0})
    {
        renderNode_->setBackgroundColor(0xFF00FF00)
        ->setSize(600, 500);
        LOGI("InteropWrapView::InteropWrapView %{public}p", renderNode_.get());
    }

    InteropWrapView::~InteropWrapView() {
        releaseMixedViewRef();
    }

    void InteropWrapView::releaseMixedViewRef() {
        if (m_mixedViewRef != nullptr && m_env != nullptr) {
            napi_delete_reference(m_env, m_mixedViewRef);
            m_mixedViewRef = nullptr;
        }
    }

    void InteropWrapView::setMixedViewRef(napi_value jsView) {
        if (jsView != nullptr && m_env != nullptr) {
            napi_status status = napi_create_reference(m_env, jsView, 1, &m_mixedViewRef);
            if (status != napi_ok) {
                LOGE("InteropWrapView::setMixedViewRef: Failed to create reference, status=%d", status);
                m_mixedViewRef = nullptr;
            }
        }
    }

    ArkUI_NodeHandle InteropWrapView::CreateMixedNode(const char* name, napi_value parameter) {
        napi_escapable_handle_scope scope;
        napi_open_escapable_handle_scope(m_env, &scope);
        napi_value result = nullptr;
        napi_value composeName = stringToNAPIValue(m_env, name);
        napi_value argv[2] = {composeName, parameter};
        napi_value createArkUIView = nullptr;
        napi_get_reference_value(m_env, m_createArkUIView, &createArkUIView);

        napi_call_function(m_env, nullptr, createArkUIView, 2, argv, &result);
        OH_ArkUI_GetNodeHandleFromNapiValue(m_env, result, &m_mixedHandle);
        maybeThrow(OH_ArkUI_NativeModule_AdoptChild(m_customNodeHandle, m_mixedHandle));
        ArkUI_RenderNodeHandle mixedRenderNode = nullptr;
        maybeThrow(OH_ArkUI_RenderNodeUtils_GetRenderNode(m_mixedHandle, &mixedRenderNode));
        m_mixedRenderNode = std::make_unique<OH::BaseRenderNode>(mixedRenderNode);
        
        napi_value escaped_result;
        napi_escape_handle(m_env, scope, result, &escaped_result);
        napi_close_escapable_handle_scope(m_env, scope);
        
        // Save reference to the JS ArkUIView object for later use in measure and getMeasuredWidth/Height
        setMixedViewRef(escaped_result);
        
        return m_mixedHandle;
    }

    OH::BaseRenderNode* InteropWrapView::getMixedRendNode() {
        if (m_mixedRenderNode) {
            return m_mixedRenderNode.get();
        } else {
            return renderNode_.get();
        }
    }

    napi_value InteropWrapView::callArkUIVIewMethod(napi_env env, napi_value object, const char* method_name,
        size_t argc, napi_value* argv) {
        napi_value method;
        napi_value method_key;

            napi_create_string_utf8(env, method_name, NAPI_AUTO_LENGTH, &method_key);
            napi_get_property(env, object, method_key, &method);

        napi_valuetype value_type;
        napi_typeof(env, method, &value_type);

            if (value_type != napi_function) {
                napi_value undefined;
                napi_get_undefined(env, &undefined);
                return undefined;
            }

        napi_value result;
        napi_call_function(env, object, method, argc, argv, &result);

        return result;
    }

    napi_value InteropWrapView::getArkUIViewProperty(napi_env env, napi_value object, const char* property_name) {
        napi_value property_value;
        napi_value key;

        napi_create_string_utf8(env, property_name, NAPI_AUTO_LENGTH, &key);

        napi_status status = napi_get_property(env, object, key, &property_value);

            if (status != napi_ok) {
                napi_get_undefined(env, &property_value);
            }

        return property_value;
    }

    void InteropWrapView::Initialize(napi_env env, napi_ref createArkUIView, ArkUI_NodeHandle customNodeHandle) {
        m_env = env;
        m_createArkUIView = createArkUIView;
        m_customNodeHandle = customNodeHandle;
    }


    napi_value InteropWrapView::stringToNAPIValue(napi_env env, const char* str) {
        napi_value result;
        napi_status status;

        if (str == NULL) {
            napi_get_null(env, &result);
            return result;
        }

        status = napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &result);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        }

        return result;
    }

    void InteropWrapView::onTouchEvent(const struct TouchEvent& event) {
        // Handle touch event
        // TODO: Implement actual touch event handling logic
        // This could dispatch to Compose's gesture system or handle directly
        LOGI("InteropWrapView::onTouchEvent: action=%{public}d, touches=%{public}zu",
             event.action, event.activeTouchPoints.size());
        
        // Example: Log touch points
        for (const auto& touch : event.activeTouchPoints) {
            LOGI("  Touch id=%{public}d, nodeX=%{public}d, nodeY=%{public}d, screenX=%{public}d, screenY=%{public}d",
                 touch.id, touch.nodeX, touch.nodeY, touch.screenX, touch.screenY);
        }

        if (m_shouldHandlerTouch) {
            for (auto item : event.activeTouchPoints) {
                OH_ArkUI_PointerEvent_SetClonedEventLocalPosition(event.cloneEvent, item.nodeX, item.nodeY);
                if (m_mixedHandle) {
                    OH_ArkUI_PointerEvent_PostClonedEvent(m_mixedHandle, event.cloneEvent);
                }
            }
        }
    }

    napi_value InteropWrapView::getJsArkUIView() {
        if (m_mixedViewRef == nullptr || m_env == nullptr) {
            return nullptr;
        }
        napi_value jsView = nullptr;
        napi_get_reference_value(m_env, m_mixedViewRef, &jsView);
        return jsView;
    }
} // namespace OH