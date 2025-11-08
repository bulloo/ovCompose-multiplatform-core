package androidx.compose.ui.interop

import androidx.compose.ui.input.pointer.PointerEventType

enum class UITouchesEventPhase {
    BEGAN, ENDED, MOVED, CANCELLED, REDIRECTED
}

fun UITouchesEventPhase.toPointerEventType(): PointerEventType =
    when (this) {
        UITouchesEventPhase.BEGAN -> PointerEventType.Press
        UITouchesEventPhase.MOVED -> PointerEventType.Move
        UITouchesEventPhase.ENDED -> PointerEventType.Release
        UITouchesEventPhase.CANCELLED -> PointerEventType.Release
        UITouchesEventPhase.REDIRECTED -> PointerEventType.Release
    }