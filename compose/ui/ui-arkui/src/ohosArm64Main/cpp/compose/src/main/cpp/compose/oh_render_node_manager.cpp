#include "oh_render_node_manager.h"
#include "xcomponent_log.h"
#include "native_node_api.h"
#include <window_manager/oh_display_manager.h>

void OHRenderNodeManager::DestroyNativeRoot() {
    // Remove all InteropWrapViews from touch event handler before destroying
    if (m_touchEventHandler) {
        for (const auto& view : m_interopWrapViews) {
            if (view != nullptr) {
                m_touchEventHandler->removeInteropWrapView(view.get());
            }
        }
    }
    
    // Clear all views
    m_interopWrapViews.clear();
    
    // Destroy touch event handler
    m_touchEventHandler.reset();
    
    if (m_contentHandle && m_customNodeHandle) {
        OH_ArkUI_NodeContent_RemoveNode(m_contentHandle, m_customNodeHandle);
    }
    OH_ArkUI_RenderNodeUtils_RemoveRenderNode(m_customNodeHandle, m_renderRootNode->getHandle());
    NativeNodeApi::getInstance()->disposeNode(m_customNodeHandle);
}

void OHRenderNodeManager::onResize(int32_t width, int32_t height) {
    float scaledDensity;

    ArkUI_NumberValue widthValue[] = {
            static_cast<float>(width)};
    ArkUI_AttributeItem widthItem = {
            widthValue, sizeof(widthValue) / sizeof(ArkUI_NumberValue)};

    NativeNodeApi::getInstance()->setLengthMetricUnit(m_customNodeHandle, ArkUI_LengthMetricUnit::ARKUI_LENGTH_METRIC_UNIT_PX);
    NativeNodeApi::getInstance()->setAttribute(m_customNodeHandle, NODE_WIDTH, &widthItem);

    ArkUI_NumberValue heightValue[] = {
            static_cast<float>(height)};
    ArkUI_AttributeItem heightItem = {
            heightValue, sizeof(heightValue) / sizeof(ArkUI_NumberValue)};

    NativeNodeApi::getInstance()->setAttribute(m_customNodeHandle, NODE_HEIGHT, &heightItem);
    m_renderRootNode->setSize(static_cast<int32_t>(width), static_cast<int32_t>(height));
}

void OHRenderNodeManager::CreateNativeRoot(napi_env env, napi_value nodeContent) {
    auto result = OH_ArkUI_GetNodeContentFromNapiValue(env, nodeContent, &m_contentHandle);
    if (result != ARKUI_ERROR_CODE_NO_ERROR || m_contentHandle == nullptr) {
        LOGE("Failed to get node content from napi value");
        return;
    }

    m_customNodeHandle = NativeNodeApi::getInstance()->createNode(ARKUI_NODE_CUSTOM);
    if (m_customNodeHandle == nullptr) {
        LOGE("Failed to create custom node");
        return;
    }

    m_renderRootNode = std::make_unique<OH::BaseRenderNode>();
    OH_ArkUI_RenderNodeUtils_AddRenderNode(m_customNodeHandle, m_renderRootNode->getHandle());
    OH_ArkUI_NodeContent_AddNode(m_contentHandle, m_customNodeHandle);
    
    // Register touch event handler for the custom node
    //m_touchEventHandler = std::make_unique<OH::InteropTouchEventHandler>(m_customNodeHandle);
    
    LOGI("Create native root successfully");
}

arkui_utils::OHNativeCanvasProxyFactory* OHRenderNodeManager::createNativeCanvasProxyFactory() {
    if (m_renderRootNode == nullptr) {
        LOGE("Render root node is null. Cannot create proxy factory.");
        return nullptr;
    }

    if (!m_canvasProxyFactory) {
        m_canvasProxyFactory = std::make_unique<arkui_utils::OHNativeCanvasProxyFactory>(m_renderRootNode.get());
    }
    return m_canvasProxyFactory.get();
}

void OHRenderNodeManager::RegisterArkUIViewCreator(napi_env env, napi_value createArkUiViewCallback) {
    m_env = env;
    napi_create_reference(env, createArkUiViewCallback, 1, &m_createArkUIView);
}

OH::InteropWrapView* OHRenderNodeManager::CreateMixedNode(const char* name, napi_value parameter) {
    auto interopWrapView = std::make_unique<OH::InteropWrapView>();
    interopWrapView->Initialize(m_env, m_createArkUIView, m_customNodeHandle);
    auto view = interopWrapView->CreateMixedNode(name, parameter);
    //NativeNodeApi::getInstance()->addChild(m_customNodeHandle, view);
//
    OH::InteropWrapView* rawPtr = interopWrapView.get();
    m_interopWrapViews.push_back(std::move(interopWrapView));

    // Add the InteropWrapView to the touch event handler for event dispatch
    // This supports multiple nodes - events will be forwarded to all registered views
    if (m_touchEventHandler) {
        m_touchEventHandler->addInteropWrapView(rawPtr);
    }

    return rawPtr;
}

