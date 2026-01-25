#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

	if (!begin || !end || end <= begin || !needle)
		return 0;

	rangeLen = (size_t)(end - begin);
	// Simple scan without allocations.
	for (size_t i = 0; i + strlen(needle) <= rangeLen; i++) {
		if (memcmp(begin + i, needle, strlen(needle)) == 0)
			return 1;
	}
	return 0;
}

static void test_CL_ClientShutdown_calls_shutdown_safe_config_writer(void)
{
	char *src;
	const char *bodyBegin;
	const char *bodyEnd;
	const char *pIf;
	const char *pCall;

	src = ReadWholeFile("../client/cl_main.c");
	if (!src) {
		TEST_FAIL_MESSAGE("failed to read ../client/cl_main.c");
		return;
	}

	if (!FindFunctionBodyRange(src, "void CL_ClientShutdown", &bodyBegin, &bodyEnd)) {
		free(src);
		TEST_FAIL_MESSAGE("could not locate CL_ClientShutdown() body");
		return;
	}

	// Be tolerant of formatting differences like "CL_WriteConfig_Default ();".
	pCall = strstr(bodyBegin, "CL_WriteConfig_Default");
	TEST_ASSERT_NOT_NULL(pCall);

	// Prefer that it only runs on clean shutdown.
	pIf = strstr(bodyBegin, "if (!errorShutdown");
	if (pIf) {
		TEST_ASSERT_NOT_NULL(strstr(pIf, "CL_WriteConfig_Default"));
	}

	free(src);
}

static void test_sb_favorites_is_archived_cvar(void)
{
	FILE *f;
	char line[2048];
	int found;

	f = fopen("../cgame/menu/m_mp_serverbrowser.c", "rb");
	if (!f) {
		TEST_FAIL_MESSAGE("failed to open ../cgame/menu/m_mp_serverbrowser.c");
		return;
	}

	found = 0;
	while (fgets(line, (int)sizeof(line), f) != NULL) {
		if (strstr(line, "Cvar_Register") && strstr(line, "\"sb_favorites\"") && strstr(line, "CVAR_ARCHIVE")) {
			found = 1;
			break;
		}
	}
	fclose(f);

	TEST_ASSERT_TRUE(found != 0);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_CL_ClientShutdown_calls_shutdown_safe_config_writer);
	RUN_TEST(test_sb_favorites_is_archived_cvar);
	return UNITY_END();
}
