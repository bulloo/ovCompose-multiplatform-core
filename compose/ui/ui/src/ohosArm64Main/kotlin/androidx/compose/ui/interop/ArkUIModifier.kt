/*
 * Tencent is pleased to support the open source community by making ovCompose available.
 * Copyright (C) 2025 Tencent. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package androidx.compose.ui.interop

import androidx.compose.runtime.Stable
import androidx.compose.ui.InternalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.arkui.ArkUIView
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.GraphicsLayerScope
import androidx.compose.ui.graphics.drawscope.ContentDrawScope
import androidx.compose.ui.layout.Measurable
import androidx.compose.ui.layout.MeasureResult
import androidx.compose.ui.layout.MeasureScope
import androidx.compose.ui.node.DrawModifierNode
import androidx.compose.ui.node.LayoutModifierNode
import androidx.compose.ui.node.ModifierNodeElement
import androidx.compose.ui.node.Nodes
import androidx.compose.ui.node.requireCoordinator
import androidx.compose.ui.platform.InspectorInfo
import androidx.compose.ui.platform.ArkUIRenderNodeLayer
import androidx.compose.ui.unit.Constraints
import androidx.compose.ui.platform.nativefoundation.OHNativeCanvasProxy

/**
 * Designed for internal use only.
 */
@InternalComposeUiApi
@Stable
internal fun Modifier.arkUILayer(viewCallback: (canvas: OHNativeCanvasProxy) -> Unit) =
    this then ArkUINodeElement(viewCallback)

private data class ArkUINodeElement(
    val viewCallback: (canvas: OHNativeCanvasProxy) -> Unit
) : ModifierNodeElement<ArkUIModifier>() {
    override fun create(): ArkUIModifier {
        return ArkUIModifier(viewCallback)
    }

    override fun update(node: ArkUIModifier) {
        node.viewCallback = viewCallback
    }

    override fun InspectorInfo.inspectableProperties() {
        name = "uiViewLayer"
    }
}

private class ArkUIModifier(
    var viewCallback: (canvas: OHNativeCanvasProxy) -> Unit
) : LayoutModifierNode, Modifier.Node() {

    override val shouldAutoInvalidate: Boolean get() = false

    val layerBlock: GraphicsLayerScope.() -> Unit = {}

    override fun MeasureScope.measure(
        measurable: Measurable,
        constraints: Constraints
    ): MeasureResult {
        val placeable = measurable.measure(constraints)
        return layout(placeable.width, placeable.height) {
            placeable.placeWithLayer(0, 0, layerBlock = layerBlock)
            (requireCoordinator(Nodes.Layout).wrapped?.layer as? ArkUIRenderNodeLayer)?.nativeCanvasProxy?.let(viewCallback)
        }
    }

    override fun toString(): String = buildString {
        append("UIViewLayerModifier")
    }
}

@InternalComposeUiApi
@Stable
internal fun Modifier.drawLayer(canvasCallback: (canvas: Canvas) -> Unit) =
    this then DrawLayerElement(canvasCallback)

private data class DrawLayerElement(
    val canvasCallback: (canvas: Canvas) -> Unit
) : ModifierNodeElement<DrawLayerModifier>() {
    override fun create(): DrawLayerModifier {
        return DrawLayerModifier(canvasCallback)
    }

    override fun update(node: DrawLayerModifier) {
        node.canvasCallback = canvasCallback
    }

    override fun InspectorInfo.inspectableProperties() {
        name = "drawLayer"
    }
}

private class DrawLayerModifier(
    var canvasCallback: (canvas: Canvas) -> Unit
) : DrawModifierNode, Modifier.Node() {

    override fun ContentDrawScope.draw() {
        drawContext.canvas.let(canvasCallback)
        drawContent()
    }

    override fun toString(): String = buildString {
        append("DrawLayerModifier")
    }
}
