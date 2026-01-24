/*
====================
test_files.c

Unit tests for FS_CreatePath, Com_NormalizePath, FS_FileExists
====================
*/

#include "../unity/unity.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <direct.h>
#include <stdarg.h>
#include <malloc.h>

typedef struct fsLink_s {
	struct fsLink_s *next;
	char *from;
	size_t fromLength;
	char *to;
} fsLink_t;

typedef struct fsPath_s {
	char pathName[260];
	char gamePath[260];
	void *package;
	struct fsPath_s *next;
} fsPath_t;

typedef struct cVar_s {
	char *string;
	int intVal;
} cVar_t;

char fs_gameDir[260] = "";

cVar_t fs_basedir_s = { .string = "." };
cVar_t *fs_basedir = &fs_basedir_s;

cVar_t fs_developer_s = { .intVal = 0 };
cVar_t *fs_developer = &fs_developer_s;

fsLink_t *fs_fileLinks = NULL;

fsPath_t fs_dummy_path = { .pathName = "." };
fsPath_t *fs_searchPaths = &fs_dummy_path;

void Sys_mkdir(char *path) {
	_mkdir(path);
}

void Com_Printf(int flags, char *fmt, ...) { (void)flags; (void)fmt; }

void Com_Error(int code, char *fmt, ...) { (void)code; (void)fmt; exit(1); }

void *Mem_PoolAlloc(size_t size, void *pool, int flags) { (void)pool; (void)flags; return malloc(size); }

void Mem_Free(void *p) { free(p); }

char *Mem_PoolStrDup(char *s, void *pool, int flags) { (void)pool; (void)flags; return strdup(s); }

uint32 Com_HashFileName(char *name, int size) { (void)name; (void)size; return 0; }

void Q_snprintfz(char *dest, size_t size, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(dest, size, fmt, ap);
	va_end(ap);
	dest[size-1] = 0;
}

#include "../common/files.c"
#include "../shared/shared.c"

void test_Com_NormalizePath_slashes_and_trailing(void) {
    char out[256];
    Com_NormalizePath(out, sizeof(out), "//foo\\bar\\baz/./../file.txt/");
    // Normalized path should not contain backslashes or "." or ".."
    TEST_ASSERT_TRUE(strchr(out, '\\') == NULL);
    TEST_ASSERT_TRUE(strstr(out, "..") == NULL);
}

void test_FS_CreatePath_and_FS_FileExists(void) {
    char path[260];
    snprintf(path, sizeof(path), "test_fs_dir/subdir/testfile.txt");

    // Ensure base dir exists by calling FS_CreatePath
    FS_CreatePath(path);

    // Create the file
    FILE *f = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "hello");
    fclose(f);

    // FS_FileExists should return positive length
    int len = FS_FileExists(path);
    TEST_ASSERT_TRUE(len > 0);

    // Cleanup
    remove(path);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Com_NormalizePath_slashes_and_trailing);
    RUN_TEST(test_FS_CreatePath_and_FS_FileExists);
    return UNITY_END();
}
