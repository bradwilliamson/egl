#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../client/cl_config_write.h"

static void Stub_Key_WriteBindings(FILE *f)
{
	fprintf(f, "bind \"f\" \"+use\"\n");
}

static void Stub_Cvar_WriteVariables(FILE *f)
{
	fprintf(f, "seta sb_favorites \"1.2.3.4:27910\"\n");
}

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

static void test_CL_WriteConfigFile_writes_cvars_section_and_favorites(void)
{
	const char *path = "test_eglcfg.cfg";
	char *txt;
	int ok;

	remove(path);
	ok = CL_WriteConfigFile(path, Stub_Key_WriteBindings, Stub_Cvar_WriteVariables);
	TEST_ASSERT_TRUE(ok != 0);

	txt = ReadWholeFile(path);
	TEST_ASSERT_NOT_NULL(txt);

	TEST_ASSERT_NOT_NULL(strstr(txt, "// console variables"));
	TEST_ASSERT_NOT_NULL(strstr(txt, "seta sb_favorites \"1.2.3.4:27910\""));

	free(txt);
	remove(path);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_CL_WriteConfigFile_writes_cvars_section_and_favorites);
	return UNITY_END();
}
