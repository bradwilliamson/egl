#include "sb_layout.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

float SB_TextWidthPx_FromGlyphCount(int glyphCount, float charW)
{
	if (glyphCount <= 0 || charW <= 0.0f)
		return 0.0f;
	return (float)glyphCount * charW;
}

float SB_RightAlignTextX(float colX, float colW, float paddingRight, float textWidthPx, float minLeftPadding)
{
	float drawX;
	if (colW <= 0.0f)
		return colX;
	if (paddingRight < 0.0f)
		paddingRight = 0.0f;
	if (minLeftPadding < 0.0f)
		minLeftPadding = 0.0f;

	drawX = (colX + colW - paddingRight) - textWidthPx;
	if (drawX < colX + minLeftPadding)
		drawX = colX + minLeftPadding;
	return drawX;
}

static void SB_Append(char *out, int outSize, const char *s)
{
	int used;
	int remaining;

	if (!out || outSize <= 0 || !s)
		return;

	used = (int)strlen(out);
	if (used >= outSize)
		return;
	remaining = outSize - used;
	if (remaining <= 1)
		return;

	// strncat always terminates if remaining > 0.
	strncat(out, s, (size_t)(remaining - 1));
}

int SB_BuildMarkerPrefix(char *out, int outSize, int showMarkers, int hasPassword, int hasAnticheat)
{
	if (!out || outSize <= 0)
		return 0;
	out[0] = '\0';

	if (!showMarkers)
		return 0;

	// Match the in-game UI: password first, then anticheat.
	if (hasPassword)
		SB_Append(out, outSize, "^3P^7 ");
	if (hasAnticheat)
		SB_Append(out, outSize, "^2A^7 ");

	return (int)strlen(out);
}

int SB_HeaderHitTestColumn(float headerX, const float *colW, int numCols, float mouseX)
{
	float curX;
	int col;

	if (!colW || numCols <= 0)
		return -1;

	curX = headerX;
	for (col = 0; col < numCols; col++) {
		if (colW[col] <= 0.0f)
			continue;
		if (mouseX >= curX && mouseX < (curX + colW[col]))
			return col;
		curX += colW[col];
	}

	return -1;
}

float SB_ScaleFromCharW(float charW, float baseCharW, float fallbackScale)
{
	if (baseCharW <= 0.0f)
		return fallbackScale;
	if (charW <= 0.0f)
		return fallbackScale;
	return charW / baseCharW;
}

float SB_ScaleFromCharCell8(float charW, float fallbackScale)
{
	return SB_ScaleFromCharW(charW, 8.0f, fallbackScale);
}

static int SB_MaxCharsForPx(float widthPx, float charW, int minChars)
{
	int chars;
	if (charW <= 0.0f)
		chars = 0;
	else
		chars = (int)(widthPx / charW);
	if (chars < minChars)
		chars = minChars;
	return chars;
}

int SB_InfoPanelMaxKeyChars(float keyColW, float charW)
{
	// Mirrors in-game: (keyColW - 12) / charW, clamped to >= 4.
	return SB_MaxCharsForPx(keyColW - 12.0f, charW, 4);
}

int SB_InfoPanelMaxValueChars(float panelW, float keyColW, float charW, int hasScrollbar)
{
	// Mirrors in-game: ((panelW - keyColW) - 12 - gutter) / charW, clamped to >= 4.
	const float gutterPx = hasScrollbar ? 10.0f : 0.0f;
	return SB_MaxCharsForPx(((panelW - keyColW) - 12.0f) - gutterPx, charW, 4);
}

int SB_ShouldApplyDeferredSort(int sortDirty, int nowMs, int lastSortMs, int delayMs)
{
	if (!sortDirty)
		return 0;
	if (delayMs < 0)
		delayMs = 0;
	return (nowMs - lastSortMs) > delayMs;
}
