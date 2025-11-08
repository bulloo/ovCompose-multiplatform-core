#include "oh_interop_touch_event_handler.h"
#include "oh_native_interop_wrap_node.h"
#include "../xcomponent_log.h"

namespace OH {

void InteropTouchEventHandler::receiveEvent(ArkUI_NodeEvent* event) {
    try {
        auto eventType = OH_ArkUI_NodeEvent_GetEventType(event);
        auto handler = static_cast<InteropTouchEventHandler*>(
            OH_ArkUI_NodeEvent_GetUserData(event));

        if (eventType == ArkUI_NodeEventType::NODE_TOUCH_EVENT) {
            auto inputEvent = OH_ArkUI_NodeEvent_GetInputEvent(event);
            if (inputEvent == nullptr ||
                OH_ArkUI_UIInputEvent_GetType(inputEvent) !=
                    ArkUI_UIInputEvent_Type::ARKUI_UIINPUTEVENT_TYPE_TOUCH) {
                return;
            }

            if (handler != nullptr) {
                handler->onTouchEvent(inputEvent);
            }
            return;
        }
    } catch (std::exception& e) {
        LOGE("InteropTouchEventHandler::receiveEvent exception: %s", e.what());
    }
}

int32_t InteropTouchEventHandler::generatedTouchPointIdentifier(ArkUI_UIInputEvent* event, uint32_t idx) {
    auto pointerId = OH_ArkUI_PointerEvent_GetPointerId(event, idx);
    // Map pointer ID similar to React Native implementation
    constexpr int32_t ARKUI_MOUSE_POINTER_ID = 1001;
    constexpr int32_t RN_MOUSE_POINTER_ID = 0;
    constexpr int32_t TOUCH_IDENTIFIER_POOL_OFFSET = 1;
    
    if (pointerId == ARKUI_MOUSE_POINTER_ID) {
        return RN_MOUSE_POINTER_ID;
    } else {
        return pointerId + TOUCH_IDENTIFIER_POOL_OFFSET;
    }
}

std::vector<TouchPoint> InteropTouchEventHandler::getTouchesFromUIInputEvent(ArkUI_UIInputEvent* event) {
    std::vector<TouchPoint> result;
    auto touchPointCount = OH_ArkUI_PointerEvent_GetPointerCount(event);
    result.reserve(touchPointCount);
    for (auto idx = 0u; idx < touchPointCount; idx++) {
        result.emplace_back(TouchPoint{
            .id = generatedTouchPointIdentifier(event, idx),
            .force = OH_ArkUI_PointerEvent_GetPressure(event, idx),
            .nodeX = int32_t(OH_ArkUI_PointerEvent_GetXByIndex(event, idx)),
            .nodeY = int32_t(OH_ArkUI_PointerEvent_GetYByIndex(event, idx)),
            .screenX = int32_t(OH_ArkUI_PointerEvent_GetDisplayXByIndex(event, idx)),
            .screenY = int32_t(OH_ArkUI_PointerEvent_GetDisplayYByIndex(event, idx))});
    }
    return result;
}

TouchPoint InteropTouchEventHandler::getActiveTouchFromUIInputEvent(ArkUI_UIInputEvent* event) {
    TouchPoint actionTouch{};
    auto screenX = int32_t(OH_ArkUI_PointerEvent_GetDisplayX(event));
    auto screenY = int32_t(OH_ArkUI_PointerEvent_GetDisplayY(event));
    auto touchPointCount = OH_ArkUI_PointerEvent_GetPointerCount(event);

    for (auto idx = 0u; idx < touchPointCount; idx++) {
        if (screenX == int32_t(OH_ArkUI_PointerEvent_GetDisplayXByIndex(event, idx)) &&
            screenY == int32_t(OH_ArkUI_PointerEvent_GetDisplayYByIndex(event, idx))) {
            actionTouch = TouchPoint{
                .id = generatedTouchPointIdentifier(event, idx),
                .force = OH_ArkUI_PointerEvent_GetPressure(event, idx),
                .nodeX = int32_t(OH_ArkUI_PointerEvent_GetX(event)),
                .nodeY = int32_t(OH_ArkUI_PointerEvent_GetY(event)),
                .screenX = int32_t(OH_ArkUI_PointerEvent_GetDisplayX(event)),
                .screenY = int32_t(OH_ArkUI_PointerEvent_GetDisplayY(event))};
            break;
        }
    }
    return actionTouch;
}

TouchEvent::TouchEvent(ArkUI_UIInputEvent* event) {
    this->action = OH_ArkUI_UIInputEvent_GetAction(event);
    this->timestamp = OH_ArkUI_UIInputEvent_GetEventTime(event);

    if (action == UI_TOUCH_EVENT_ACTION_MOVE) {
        // For MOVE events, we need all touches
        // We'll get them from the handler
        this->activeTouchPoints = {};
    } else {
        // For DOWN, UP, CANCEL events, we only need the active touch
        this->activeTouchPoints = {};
    }
    OH_ArkUI_PointerEvent_CreateClonedEvent(event, &(this->cloneEvent));
}

InteropTouchEventHandler::InteropTouchEventHandler(ArkUI_NodeHandle nodeHandle)
    : m_nodeHandle(nodeHandle), m_interopWrapView(nullptr) {
    if (m_nodeHandle == nullptr) {
        LOGE("InteropTouchEventHandler: nodeHandle is null");
        return;
    }

    auto api = NativeNodeApi::getInstance();
    if (api == nullptr) {
        LOGE("InteropTouchEventHandler: NativeNodeApi is null");
        return;
    }

    api->addNodeEventReceiver(m_nodeHandle, receiveEvent);
    api->registerNodeEvent(m_nodeHandle, NODE_TOUCH_EVENT, NODE_TOUCH_EVENT, this);
    LOGI("InteropTouchEventHandler: registered touch events for node");
}

InteropTouchEventHandler::~InteropTouchEventHandler() noexcept {
    if (m_nodeHandle == nullptr) {
        return;
    }

    auto api = NativeNodeApi::getInstance();
    if (api == nullptr) {
        return;
    }

    api->unregisterNodeEvent(m_nodeHandle, NODE_TOUCH_EVENT);
    api->removeNodeEventReceiver(m_nodeHandle, receiveEvent);
    LOGI("InteropTouchEventHandler: unregistered touch events");
}

void InteropTouchEventHandler::setInteropWrapView(InteropWrapView* view) {
    m_interopWrapView = view;
}

void InteropTouchEventHandler::onTouchEvent(ArkUI_UIInputEvent* event) {
    if (event == nullptr) {
        return;
    }

    if (m_interopWrapView == nullptr) {
        // No view to dispatch to, but we can still log
        LOGI("InteropTouchEventHandler::onTouchEvent: no InteropWrapView set");
        return;
    }

    TouchEvent touchEvent(event);
    
    // Get all touches for MOVE events, or active touch for others
    if (touchEvent.action == UI_TOUCH_EVENT_ACTION_MOVE) {
        touchEvent.activeTouchPoints = getTouchesFromUIInputEvent(event);
    } else {
        touchEvent.activeTouchPoints = {getActiveTouchFromUIInputEvent(event)};
    }

    // Dispatch to InteropWrapView
    if (m_interopWrapView != nullptr) {
        m_interopWrapView->onTouchEvent(touchEvent);
    } else {
        LOGI("InteropTouchEventHandler::onTouchEvent: no InteropWrapView set, action=%d, touches=%zu", 
             touchEvent.action, touchEvent.activeTouchPoints.size());
    }
}

} // namespace OH

