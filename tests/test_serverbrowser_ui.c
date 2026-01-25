#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sb_layout.h"

static char *ReadWholeFile(const char *path)
{
	FILE *f;
	long len;
	size_t nread;
	char *buf;

	f = fopen(path, "rb");
	if (!f)
		return NULL;
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return NULL;
	}
	len = ftell(f);
	if (len < 0) {
		fclose(f);
		return NULL;
	}
	if (fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return NULL;
	}
	buf = (char *)malloc((size_t)len + 1);
	if (!buf) {
		fclose(f);
		return NULL;
	}
	nread = fread(buf, 1, (size_t)len, f);
	fclose(f);
	buf[nread] = '\0';
	return buf;
}

static int FindFunctionBodyRange(const char *src, const char *needle, const char **outBegin, const char **outEnd)
{
	const char *start;
	const char *p;
	int depth;

	if (!src || !needle || !outBegin || !outEnd)
		return 0;

	start = strstr(src, needle);
	if (!start)
		return 0;

	// Find the first '{' after the signature.
	p = start;
	while (*p && *p != '{')
		p++;
	if (*p != '{')
		return 0;

	*outBegin = p;
	depth = 0;
	for (; *p; p++) {
		if (*p == '{')
			depth++;
		else if (*p == '}') {
			depth--;
			if (depth == 0) {
				*outEnd = p + 1;
				return 1;
			}
		}
	}

	return 0;
}

static int ContainsSubstringInRange(const char *begin, const char *end, const char *needle)
{
	size_t rangeLen;
	size_t needleLen;

	if (!begin || !end || end <= begin || !needle)
		return 0;

	needleLen = strlen(needle);
	if (!needleLen)
		return 1;

	rangeLen = (size_t)(end - begin);
	for (size_t i = 0; i + needleLen <= rangeLen; i++) {
		if (memcmp(begin + i, needle, needleLen) == 0)
			return 1;
	}
	return 0;
}

static void test_SB_TextWidthPx_FromGlyphCount_basic(void)
{
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, SB_TextWidthPx_FromGlyphCount(3, 10.0f));
}

static void test_SB_RightAlignTextX_uses_pixel_width(void)
{
	const float colX = 100.0f;
	const float colW = 50.0f;
	const float textW = SB_TextWidthPx_FromGlyphCount(3, 10.0f);

	// (100 + 50 - 2) - 30 = 118
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 118.0f, SB_RightAlignTextX(colX, colW, 2.0f, textW, 2.0f));
}

static void test_SB_RightAlignTextX_clamps_to_left_padding(void)
{
	const float colX = 100.0f;
	const float colW = 50.0f;
	const float textW = SB_TextWidthPx_FromGlyphCount(10, 10.0f); // 100px, wider than colW

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 102.0f, SB_RightAlignTextX(colX, colW, 2.0f, textW, 2.0f));
}

static void test_SB_BuildMarkerPrefix_respects_toggle_and_order(void)
{
	char prefix[64];

	SB_BuildMarkerPrefix(prefix, (int)sizeof(prefix), 0, 1, 1);
	TEST_ASSERT_EQUAL_STRING("", prefix);

	SB_BuildMarkerPrefix(prefix, (int)sizeof(prefix), 1, 1, 1);
	TEST_ASSERT_EQUAL_STRING("^3P^7 ^2A^7 ", prefix);
}

static void test_SB_HeaderHitTestColumn_skips_hidden_columns(void)
{
	// Visible columns are: 0 (10px), 2 (20px), 4 (30px)
	const float colW[] = { 10.0f, 0.0f, 20.0f, 0.0f, 30.0f };

	TEST_ASSERT_EQUAL(0, SB_HeaderHitTestColumn(100.0f, colW, 5, 105.0f));
	TEST_ASSERT_EQUAL(2, SB_HeaderHitTestColumn(100.0f, colW, 5, 115.0f));
	TEST_ASSERT_EQUAL(4, SB_HeaderHitTestColumn(100.0f, colW, 5, 135.0f));
	TEST_ASSERT_EQUAL(-1, SB_HeaderHitTestColumn(100.0f, colW, 5, 99.0f));
}

static void test_SB_ScaleFromCharW_matches_fontscale_math(void)
{
	// Generic helper: baseCharW is caller-provided.
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, SB_ScaleFromCharW(8.0f, 8.0f, 1.0f));
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, SB_ScaleFromCharW(4.8f, 8.0f, 1.0f));
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, SB_ScaleFromCharW(0.0f, 8.0f, 1.0f));
}

static void test_SB_ScaleFromCharCell8_preserves_ui_scale(void)
{
	// In-game, charW is computed from (8 * UI_SCALE) * fontScale.
	// The server browser draw scale must be based on an 8px font cell so that
	// the UI_SCALE portion remains intact.
	// Example: UI_SCALE=2, fontScale=0.6 => charW=8*2*0.6=9.6 => scale=9.6/8=1.2
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.2f, SB_ScaleFromCharCell8(9.6f, 1.0f));
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, SB_ScaleFromCharCell8(0.0f, 1.0f));
}

static void test_SB_InfoPanelMaxValueChars_reserves_scrollbar_gutter(void)
{
	// panelW=200, keyColW=80 => valueWidth starts at 200-80=120
	// subtract padding 12 => 108
	// with scrollbar gutter 10 => 98
	// charW=8 => 108/8=13, 98/8=12
	TEST_ASSERT_EQUAL(13, SB_InfoPanelMaxValueChars(200.0f, 80.0f, 8.0f, 0));
	TEST_ASSERT_EQUAL(12, SB_InfoPanelMaxValueChars(200.0f, 80.0f, 8.0f, 1));
}

static void test_SB_ShouldApplyDeferredSort_obeys_delay_and_dirty(void)
{
	TEST_ASSERT_EQUAL(0, SB_ShouldApplyDeferredSort(0, 1000, 0, 200));
	TEST_ASSERT_EQUAL(0, SB_ShouldApplyDeferredSort(1, 1000, 900, 200));
	TEST_ASSERT_TRUE(SB_ShouldApplyDeferredSort(1, 1000, 799, 200) != 0);
}

static void test_ServerBrowser_uses_SB_ScaleFromCharCell8_in_all_draw_paths(void)
{
	char *src;
	const char *bodyBegin;
	const char *bodyEnd;

	src = ReadWholeFile("../cgame/menu/m_mp_serverbrowser.c");
	if (!src) {
		TEST_FAIL_MESSAGE("failed to read ../cgame/menu/m_mp_serverbrowser.c");
		return;
	}

	// Header and row scale should use the shared helper.
	TEST_ASSERT_TRUE(FindFunctionBodyRange(src, "static void ServerBrowser_DrawHeader", &bodyBegin, &bodyEnd) != 0);
	TEST_ASSERT_TRUE(ContainsSubstringInRange(bodyBegin, bodyEnd, "SB_ScaleFromCharCell8") != 0);

	TEST_ASSERT_TRUE(FindFunctionBodyRange(src, "static void ServerBrowser_DrawServerRow", &bodyBegin, &bodyEnd) != 0);
	TEST_ASSERT_TRUE(ContainsSubstringInRange(bodyBegin, bodyEnd, "SB_ScaleFromCharCell8") != 0);

	// Right-side panels: this is the specific regression we want to prevent.
	TEST_ASSERT_TRUE(FindFunctionBodyRange(src, "static void ServerBrowser_DrawInfoPanel", &bodyBegin, &bodyEnd) != 0);
	TEST_ASSERT_TRUE(ContainsSubstringInRange(bodyBegin, bodyEnd, "SB_ScaleFromCharCell8") != 0);

	TEST_ASSERT_TRUE(FindFunctionBodyRange(src, "static void ServerBrowser_DrawPlayersPanel", &bodyBegin, &bodyEnd) != 0);
	TEST_ASSERT_TRUE(ContainsSubstringInRange(bodyBegin, bodyEnd, "SB_ScaleFromCharCell8") != 0);

	free(src);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_SB_TextWidthPx_FromGlyphCount_basic);
	RUN_TEST(test_SB_RightAlignTextX_uses_pixel_width);
	RUN_TEST(test_SB_RightAlignTextX_clamps_to_left_padding);
	RUN_TEST(test_SB_BuildMarkerPrefix_respects_toggle_and_order);
	RUN_TEST(test_SB_HeaderHitTestColumn_skips_hidden_columns);
	RUN_TEST(test_SB_ScaleFromCharW_matches_fontscale_math);
	RUN_TEST(test_SB_ScaleFromCharCell8_preserves_ui_scale);
	RUN_TEST(test_SB_InfoPanelMaxValueChars_reserves_scrollbar_gutter);
	RUN_TEST(test_SB_ShouldApplyDeferredSort_obeys_delay_and_dirty);
	RUN_TEST(test_ServerBrowser_uses_SB_ScaleFromCharCell8_in_all_draw_paths);
	return UNITY_END();
}
