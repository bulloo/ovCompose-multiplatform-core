#pragma once

#include <arkui/native_node.h>
#include <arkui/ui_input_event.h>
#include "../native_node_api.h"
#include <vector>
#include <algorithm>

namespace OH {

struct TouchPoint {
    int32_t id;
    float force;
    int32_t nodeX;
    int32_t nodeY;
    int32_t screenX;
    int32_t screenY;
};

struct TouchEvent {
    uint32_t action;
    uint64_t timestamp;
    std::vector<TouchPoint> activeTouchPoints;
    ArkUI_UIInputEvent* cloneEvent;

    explicit TouchEvent(ArkUI_UIInputEvent* event);
};

// Forward declaration
class InteropWrapView;

class InteropTouchEventHandler {
public:
    InteropTouchEventHandler(ArkUI_NodeHandle nodeHandle);
    virtual ~InteropTouchEventHandler() noexcept;
    
    void onTouchEvent(ArkUI_UIInputEvent* event);
    void addInteropWrapView(InteropWrapView* view);
    void removeInteropWrapView(InteropWrapView* view);
    
private:
    static void receiveEvent(ArkUI_NodeEvent* event);
    TouchPoint getActiveTouchFromUIInputEvent(ArkUI_UIInputEvent* event);
    std::vector<TouchPoint> getTouchesFromUIInputEvent(ArkUI_UIInputEvent* event);
    int32_t generatedTouchPointIdentifier(ArkUI_UIInputEvent* event, uint32_t idx);
    
    ArkUI_NodeHandle m_nodeHandle;
    std::vector<InteropWrapView*> m_interopWrapViews;  // Support multiple views
};

} // namespace OH

