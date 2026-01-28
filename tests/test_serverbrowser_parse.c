#include "unity.h"

#include <stdio.h>
#include <string.h>

static int is_comment_or_blank(const char *s)
{
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		s++;
	return (*s == 0 || *s == '#');
}

static int parse_text_list_fixture(const char *path)
{
	FILE *f = fopen(path, "rb");
	char line[1024];
	int count = 0;
	TEST_ASSERT_NOT_NULL(f);

	while (fgets(line, sizeof(line), f)) {
		size_t n = strlen(line);
		while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r' || line[n - 1] == ' ' || line[n - 1] == '\t'))
			line[--n] = 0;
		if (is_comment_or_blank(line))
			continue;
		if (!strncmp(line, "quake2://", 9))
			TEST_ASSERT_TRUE(strchr(line + 9, ':') != NULL);
		else
			TEST_ASSERT_TRUE(strchr(line, ':') != NULL);
		count++;
	}
	fclose(f);
	return count;
}

static int parse_binary_list_fixture(const char *path)
{
	FILE *f = fopen(path, "rb");
	unsigned char buf[4096];
	size_t len;
	int count = 0;
	TEST_ASSERT_NOT_NULL(f);
	len = fread(buf, 1, sizeof(buf), f);
	fclose(f);
	TEST_ASSERT_TRUE(len >= 6);
	TEST_ASSERT_EQUAL(0, (int)(len % 6));

	for (size_t i = 0; i + 6 <= len; i += 6) {
		unsigned ip0 = buf[i + 0];
		unsigned ip1 = buf[i + 1];
		unsigned ip2 = buf[i + 2];
		unsigned ip3 = buf[i + 3];
		unsigned portNet = (unsigned)buf[i + 4] << 8 | (unsigned)buf[i + 5];
		if (ip0 == 0 && ip1 == 0 && ip2 == 0 && ip3 == 0 && portNet == 0)
			break;
		count++;
	}
	return count;
}

void test_q2servers_raw1_text_fixture_has_two_entries(void)
{
	int count = parse_text_list_fixture("fixtures/q2servers_raw1.txt");
	TEST_ASSERT_EQUAL(2, count);
}

void test_q2servers_raw2_binary_fixture_has_two_entries(void)
{
	int count = parse_binary_list_fixture("fixtures/q2servers_raw2.bin");
	TEST_ASSERT_EQUAL(2, count);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_q2servers_raw1_text_fixture_has_two_entries);
	RUN_TEST(test_q2servers_raw2_binary_fixture_has_two_entries);
	return UNITY_END();
}
