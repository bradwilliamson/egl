#pragma once

// Pure helpers used by the in-game server browser UI.
// Kept dependency-free to enable unit testing.

#ifdef __cplusplus
extern "C" {
#endif

// Converts the glyph/character count returned by the font renderer into pixels.
float SB_TextWidthPx_FromGlyphCount(int glyphCount, float charW);

// Computes a right-aligned X position within a column.
//
// Equivalent to the in-game list logic:
//   drawX = (colX + colW - paddingRight) - textWidthPx
//   drawX = max(drawX, colX + minLeftPadding)
float SB_RightAlignTextX(float colX, float colW, float paddingRight, float textWidthPx, float minLeftPadding);

// Builds the optional colored marker prefix for a server row (e.g. password/anticheat).
// Returns the number of bytes written (excluding the null terminator).
int SB_BuildMarkerPrefix(char *out, int outSize, int showMarkers, int hasPassword, int hasAnticheat);

// Header hit test that matches rendering: columns with width <= 0 are skipped.
// Returns the clicked column index, or -1 if none.
int SB_HeaderHitTestColumn(float headerX, const float *colW, int numCols, float mouseX);

// Derives a font-render scale from the UI character cell width.
// In the server browser we compute:
//   charW = baseCharW * fontScale
// So:
//   fontScale = charW / baseCharW
float SB_ScaleFromCharW(float charW, float baseCharW, float fallbackScale);

// Server browser UI text uses a baseline 8px cell for scale calculations.
// This intentionally preserves any UI scaling that is already baked into charW.
float SB_ScaleFromCharCell8(float charW, float fallbackScale);

// Computes the max number of visible characters for the server info panel columns.
// These helpers intentionally mirror the in-game constants (padding and min chars)
// so regressions are caught by unit tests.
int SB_InfoPanelMaxKeyChars(float keyColW, float charW);
int SB_InfoPanelMaxValueChars(float panelW, float keyColW, float charW, int hasScrollbar);

// Pure timing helper for deferred sort logic.
// Returns non-zero if a deferred sort should run now.
int SB_ShouldApplyDeferredSort(int sortDirty, int nowMs, int lastSortMs, int delayMs);

#ifdef __cplusplus
}
#endif
