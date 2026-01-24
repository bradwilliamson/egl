/*
 * test_memory.c - Unit tests for memory pool allocator (Mem_Alloc, Mem_Free)
 * Tests allocation, freeing, tag management, and leak detection
 */

#include "../unity/unity.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#endif

// Types
typedef int qBool;
#define qTrue 1
#define qFalse 0

typedef unsigned char byte;
typedef unsigned int uint32;

// Sentinel values
#define MEM_HEAD_SENTINEL_TOP	0xFEBDFAED
#define MEM_HEAD_SENTINEL_BOT	0xD0BAF0FF
#define MEM_FOOT_SENTINEL		0xF00DF00D

typedef struct memBlockFoot_s {
	uint32				sentinel;
} memBlockFoot_t;

typedef struct memBlock_s {
	struct memBlock_s	*next;
	uint32				topSentinel;
	struct memPool_s	*pool;
	int					tagNum;
	size_t				size;
	const char			*allocFile;
	int					allocLine;
	void				*memPointer;
	size_t				memSize;
	memBlockFoot_t		*footer;
	uint32				botSentinel;
	/* For custom allocation bookkeeping */
	void			*rawAlloc;
	int				use_aligned_free;
} memBlock_t;

#define MEM_MAX_POOLCOUNT	32
#define MEM_MAX_POOLNAME	64

typedef struct memPool_s {
	char				name[MEM_MAX_POOLNAME];
	qBool				inUse;
	memBlock_t			*blocks;
	size_t				blockCount;
	size_t				byteCount;
	const char			*createFile;
	int					createLine;
} memPool_t;

// Mock error handling
static int mockErrorCalled = 0;
static void Com_Error(int code, const char *fmt, ...) {
	(void)code;
	(void)fmt;
	mockErrorCalled = 1;
}

static void Com_DevPrintf(int level, const char *fmt, ...) {
	(void)level;
	(void)fmt;
}

// Forward declarations
static memPool_t *Mem_FindPool(const char *name);
memPool_t *_Mem_CreatePool(const char *name, const char *fileName, const int fileLine);
size_t _Mem_DeletePool(struct memPool_s *pool, const char *fileName, const int fileLine);
void *_Mem_Alloc(size_t size, struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine);
size_t _Mem_Free(const void *ptr, const char *fileName, const int fileLine);
size_t _Mem_FreeTag(struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine);
size_t _Mem_FreePool(struct memPool_s *pool, const char *fileName, const int fileLine);
char *_Mem_PoolStrDup(const char *in, struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine);
size_t _Mem_PoolSize(struct memPool_s *pool);
size_t _Mem_TagSize(struct memPool_s *pool, const int tagNum);
size_t _Mem_ChangeTag(struct memPool_s *pool, const int tagFrom, const int tagTo);

// Macros
#define Mem_CreatePool(name) _Mem_CreatePool((name),__FILE__,__LINE__)
#define Mem_DeletePool(pool) _Mem_DeletePool((pool),__FILE__,__LINE__)
#define Mem_Alloc(size, pool, tag) _Mem_Alloc((size),(pool),(tag),__FILE__,__LINE__)
#define Mem_Free(ptr) _Mem_Free((ptr),__FILE__,__LINE__)
#define Mem_FreeTag(pool, tag) _Mem_FreeTag((pool),(tag),__FILE__,__LINE__)
#define Mem_FreePool(pool) _Mem_FreePool((pool),__FILE__,__LINE__)
#define Mem_StrDup(str, pool, tag) _Mem_PoolStrDup((str),(pool),(tag),__FILE__,__LINE__)
#define Mem_PoolSize(pool) _Mem_PoolSize((pool))
#define Mem_TagSize(pool, tag) _Mem_TagSize((pool),(tag))
#define Mem_ChangeTag(pool, from, to) _Mem_ChangeTag((pool),(from),(to))

// Global state
static memPool_t m_poolList[MEM_MAX_POOLCOUNT];
static uint32 m_numPools;

// Implementation
static memPool_t *Mem_FindPool(const char *name) {
	memPool_t *pool;
	uint32 i;

	for (i = 0, pool = &m_poolList[0]; i < m_numPools; pool++, i++) {
		if (!pool->inUse)
			continue;
		if (strcmp(name, pool->name))
			continue;
		return pool;
	}
	return NULL;
}

memPool_t *_Mem_CreatePool(const char *name, const char *fileName, const int fileLine) {
	memPool_t *pool;
	uint32 i;

	if (!name || !name[0]) {
		Com_Error(1, "Mem_CreatePool: NULL name");
		return NULL;
	}

	// See if it already exists
	pool = Mem_FindPool(name);
	if (pool) {
		return pool;
	}

	// Create a slot
	for (i = 0, pool = &m_poolList[0]; i < m_numPools; pool++, i++) {
		if (!pool->inUse)
			break;
	}
	if (i == m_numPools) {
		if (m_numPools + 1 >= MEM_MAX_POOLCOUNT) {
			Com_Error(1, "Mem_CreatePool: MEM_MAX_POOLCOUNT");
			return NULL;
		}
		pool = &m_poolList[m_numPools++];
	}

	// Store values
	pool->blocks = NULL;
	pool->blockCount = 0;
	pool->byteCount = 0;
	pool->createFile = fileName;
	pool->createLine = fileLine;
	pool->inUse = qTrue;
	strncpy(pool->name, name, sizeof(pool->name) - 1);
	pool->name[sizeof(pool->name) - 1] = '\0';
	return pool;
}

size_t _Mem_DeletePool(struct memPool_s *pool, const char *fileName, const int fileLine) {
	size_t size;

	if (!pool)
		return 0;

	// Release all allocated memory
	size = _Mem_FreePool(pool, fileName, fileLine);

	// Mark as unused
	pool->inUse = qFalse;
	pool->name[0] = '\0';

	return size;
}

size_t _Mem_Free(const void *ptr, const char *fileName, const int fileLine) {
	memBlock_t *mem;
	memBlock_t *search;
	memBlock_t **prev;
	size_t size;

	if (!ptr)
		return 0;


	// Check sentinels
	mem = (memBlock_t *)((byte *)ptr - sizeof(memBlock_t));
	if (mem->topSentinel != MEM_HEAD_SENTINEL_TOP) {
		Com_Error(1, "Mem_Free: bad memory header top sentinel");
		return 0;
	}
	if (mem->botSentinel != MEM_HEAD_SENTINEL_BOT) {
		Com_Error(1, "Mem_Free: bad memory header bottom sentinel");
		return 0;
	}
	if (!mem->footer) {
		Com_Error(1, "Mem_Free: bad memory footer");
		return 0;
	}
	if (mem->footer->sentinel != MEM_FOOT_SENTINEL) {
		Com_Error(1, "Mem_Free: bad memory footer sentinel");
		return 0;
	}


	// Decrement counters
	mem->pool->blockCount--;
	mem->pool->byteCount -= mem->size;
	size = mem->size;

	// De-link it
	prev = &mem->pool->blocks;
	for (;;) {
		search = *prev;
		if (!search)
			break;

		if (search == mem) {
			*prev = search->next;
			break;
		}
		prev = &search->next;
	}

	// Free it (use the original allocation pointer and matching free function)
	if (mem->rawAlloc) {
#if defined(_WIN32) || defined(_WIN64)
		if (mem->use_aligned_free) {
			_aligned_free(mem->rawAlloc);
		} else {
			free(mem->rawAlloc);
		}
#else
		free(mem->rawAlloc);
#endif
	} else {
		free(mem);
	}
	return size;
}

size_t _Mem_FreeTag(struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine) {
	memBlock_t *mem, *next;
	size_t size;

	if (!pool)
		return 0;

	size = 0;
	for (mem = pool->blocks; mem; mem = next) {
		next = mem->next;
		if (mem->tagNum == tagNum)
			size += _Mem_Free(mem->memPointer, fileName, fileLine);
	}

	return size;
}

size_t _Mem_FreePool(struct memPool_s *pool, const char *fileName, const int fileLine) {
	memBlock_t *mem, *next;
	size_t size;

	if (!pool)
		return 0;

	size = 0;
	for (mem = pool->blocks; mem; mem = next) {
		next = mem->next;
		size += _Mem_Free(mem->memPointer, fileName, fileLine);
	}

	return size;
}

void *_Mem_Alloc(size_t size, struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine) {
	memBlock_t *mem;
	size_t allocSize;

	if (!pool)
		return NULL;

	if (size <= 0) {
		Com_DevPrintf(0, "Mem_Alloc: Attempted allocation of '%zu' memory ignored\n", size);
		return NULL;
	}
	if (size > 0x40000000) {
		Com_Error(1, "Mem_Alloc: Attempted allocation of '%zu' bytes!\n", size);
		return NULL;
	}

	// Add header and round to cacheline
	allocSize = (size + sizeof(memBlock_t) + sizeof(memBlockFoot_t) + 31) & ~31;
	/* Try to allocate with 32-byte alignment. Prefer platform aligned allocs, but
	   fall back to a manual aligned allocation when necessary. */
	void *raw = NULL;
	/* orig_alloc stores the original pointer returned by the system allocator; */
	void *orig_alloc = NULL;
	int orig_use_aligned_free = 0;
#if defined(_WIN32) || defined(_WIN64)
	/* On MSVC prefer _aligned_malloc which must be freed with _aligned_free */
	#if defined(_MSC_VER)
		mem = (memBlock_t *)_aligned_malloc(allocSize, 32);
		if (mem) {
			orig_alloc = mem;
			orig_use_aligned_free = 1;
			memset(mem, 0, allocSize);
		}
	#else
		/* MinGW or other Windows toolchains: fallback to manual alignment */
		raw = malloc(allocSize + 31 + sizeof(void *));
		if (raw) {
			uintptr_t aligned = ((uintptr_t)raw + sizeof(void *) + 31) & ~((uintptr_t)31);
			mem = (memBlock_t *)aligned;
			orig_alloc = raw;
			orig_use_aligned_free = 0;
			memset(mem, 0, allocSize);
		} else {
			mem = NULL;
		}
	#endif
#else
	if (posix_memalign(&raw, 32, allocSize) == 0) {
		mem = (memBlock_t *)raw;
		orig_alloc = raw;
		orig_use_aligned_free = 0;
		memset(mem, 0, allocSize);
	} else {
		/* Fallback: allocate extra and align manually, storing original pointer */
		raw = malloc(allocSize + 31 + sizeof(void *));
		if (raw) {
			uintptr_t aligned = ((uintptr_t)raw + sizeof(void *) + 31) & ~((uintptr_t)31);
			mem = (memBlock_t *)aligned;
			orig_alloc = raw;
			orig_use_aligned_free = 0;
			memset(mem, 0, allocSize);
		}
	}
#endif

	if (!mem) {
		Com_Error(1, "Mem_Alloc: failed on allocation");
		return NULL;
	}

	// For integrity checking and stats
	pool->blockCount++;
	pool->byteCount += allocSize;

	// Fill in the header
	mem->topSentinel = MEM_HEAD_SENTINEL_TOP;
	mem->tagNum = tagNum;
	mem->size = allocSize;
	mem->memPointer = (void *)(mem + 1);
	mem->memSize = allocSize - sizeof(memBlock_t) - sizeof(memBlockFoot_t);
	mem->pool = pool;
	mem->allocFile = fileName;
	mem->allocLine = fileLine;

	mem->footer = (memBlockFoot_t *)((byte *)mem->memPointer + mem->memSize);
	mem->botSentinel = MEM_HEAD_SENTINEL_BOT;

	// Fill in the footer
	mem->footer->sentinel = MEM_FOOT_SENTINEL;

	/* Store original allocation pointer and free-mode after header/footer are set -
	they may be overwritten by the memset above. */
	mem->rawAlloc = orig_alloc ? orig_alloc : mem;
	mem->use_aligned_free = orig_use_aligned_free;

	// Link it in to the appropriate pool
	mem->next = pool->blocks;
	pool->blocks = mem;

	return mem->memPointer;
}

char *_Mem_PoolStrDup(const char *in, struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine) {
	char *out;

	out = (char *)_Mem_Alloc((size_t)(strlen(in) + 1), pool, tagNum, fileName, fileLine);
	if (out)
		strcpy(out, in);

	return out;
}

size_t _Mem_PoolSize(struct memPool_s *pool) {
	if (!pool)
		return 0;
	return pool->byteCount;
}

size_t _Mem_TagSize(struct memPool_s *pool, const int tagNum) {
	memBlock_t *mem;
	size_t size;

	if (!pool)
		return 0;

	size = 0;
	for (mem = pool->blocks; mem; mem = mem->next) {
		if (mem->tagNum == tagNum)
			size += mem->size;
	}

	return size;
}

size_t _Mem_ChangeTag(struct memPool_s *pool, const int tagFrom, const int tagTo) {
	memBlock_t *mem;
	size_t size;

	if (!pool)
		return 0;

	size = 0;
	for (mem = pool->blocks; mem; mem = mem->next) {
		if (mem->tagNum == tagFrom) {
			mem->tagNum = tagTo;
			size += mem->size;
		}
	}

	return size;
}

// Helper to reset state (called from tests as needed)
static void ResetMemoryState(void) {
	uint32 i;
	// Clean up any remaining pools
	for (i = 0; i < m_numPools; i++) {
		if (m_poolList[i].inUse) {
			_Mem_DeletePool(&m_poolList[i], __FILE__, __LINE__);
		}
	}
	memset(m_poolList, 0, sizeof(m_poolList));
	m_numPools = 0;
	mockErrorCalled = 0;
}

// ============================================================================
// TESTS: Pool Management
// ============================================================================

void test_Mem_CreatePool_Basic(void) {
	memPool_t *pool;
	
	pool = Mem_CreatePool("test_pool");
	
	TEST_ASSERT_NOT_NULL(pool);
	TEST_ASSERT_TRUE(pool->inUse);
	TEST_ASSERT_EQUAL_STRING("test_pool", pool->name);
	TEST_ASSERT_EQUAL(0, pool->blockCount);
	TEST_ASSERT_EQUAL(0, pool->byteCount);
}

void test_Mem_CreatePool_Multiple(void) {
	memPool_t *pool1, *pool2, *pool3;
	
	pool1 = Mem_CreatePool("pool1");
	pool2 = Mem_CreatePool("pool2");
	pool3 = Mem_CreatePool("pool3");
	
	TEST_ASSERT_NOT_NULL(pool1);
	TEST_ASSERT_NOT_NULL(pool2);
	TEST_ASSERT_NOT_NULL(pool3);
	TEST_ASSERT_TRUE(pool1 != pool2);
	TEST_ASSERT_TRUE(pool2 != pool3);
	TEST_ASSERT_EQUAL_STRING("pool1", pool1->name);
	TEST_ASSERT_EQUAL_STRING("pool2", pool2->name);
	TEST_ASSERT_EQUAL_STRING("pool3", pool3->name);
}

void test_Mem_CreatePool_ReturnsExisting(void) {
	memPool_t *pool1, *pool2;
	
	pool1 = Mem_CreatePool("duplicate");
	pool2 = Mem_CreatePool("duplicate");
	
	TEST_ASSERT_NOT_NULL(pool1);
	TEST_ASSERT_TRUE(pool1 == pool2); // Should return same pool
}

void test_Mem_DeletePool_EmptyPool(void) {
	memPool_t *pool;
	size_t freed;
	
	pool = Mem_CreatePool("temp_pool");
	TEST_ASSERT_TRUE(pool->inUse);
	
	freed = Mem_DeletePool(pool);
	
	TEST_ASSERT_EQUAL(0, freed);
	TEST_ASSERT_FALSE(pool->inUse);
}

// ============================================================================
// TESTS: Basic Allocation and Freeing
// ============================================================================

void test_Mem_Alloc_SimpleAllocation(void) {
	memPool_t *pool;
	void *ptr;
	
	pool = Mem_CreatePool("alloc_test");
	ptr = Mem_Alloc(100, pool, 0);
	
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL(1, pool->blockCount);
	TEST_ASSERT(pool->byteCount > 100); // Includes headers
	
	Mem_Free(ptr);
}

void test_Mem_Alloc_ZeroSize(void) {
	memPool_t *pool;
	void *ptr;
	
	pool = Mem_CreatePool("zero_test");
	ptr = Mem_Alloc(0, pool, 0);
	
	TEST_ASSERT_NULL(ptr); // Should return NULL for zero size
	TEST_ASSERT_EQUAL(0, pool->blockCount);
}

void test_Mem_Alloc_MultipleAllocations(void) {
	memPool_t *pool;
	void *ptr1, *ptr2, *ptr3;
	
	pool = Mem_CreatePool("multi_alloc");
	ptr1 = Mem_Alloc(64, pool, 0);
	ptr2 = Mem_Alloc(128, pool, 0);
	ptr3 = Mem_Alloc(256, pool, 0);
	
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_NOT_NULL(ptr2);
	TEST_ASSERT_NOT_NULL(ptr3);
	TEST_ASSERT_EQUAL(3, pool->blockCount);
	
	Mem_Free(ptr1);
	Mem_Free(ptr2);
	Mem_Free(ptr3);
}

void test_Mem_Free_DecrementsCounts(void) {
	memPool_t *pool;
	void *ptr;
	size_t initialBytes;
	
	pool = Mem_CreatePool("free_test");
	ptr = Mem_Alloc(100, pool, 0);
	initialBytes = pool->byteCount;
	
	TEST_ASSERT_EQUAL(1, pool->blockCount);
	TEST_ASSERT(initialBytes > 0);
	
	Mem_Free(ptr);
	
	TEST_ASSERT_EQUAL(0, pool->blockCount);
	TEST_ASSERT_EQUAL(0, pool->byteCount);
}

void test_Mem_Alloc_Free_NullPointer(void) {
	size_t freed;
	
	freed = Mem_Free(NULL);
	
	TEST_ASSERT_EQUAL(0, freed); // Should handle NULL gracefully
}

// ============================================================================
// TESTS: Memory Integrity (Sentinels)
// ============================================================================

void test_Mem_Alloc_SetsSentinels(void) {
	memPool_t *pool;
	void *ptr;
	memBlock_t *mem;
	
	pool = Mem_CreatePool("sentinel_test");
	ptr = Mem_Alloc(100, pool, 0);
	
	mem = (memBlock_t *)((byte *)ptr - sizeof(memBlock_t));
	
	TEST_ASSERT_EQUAL_HEX32(MEM_HEAD_SENTINEL_TOP, mem->topSentinel);
	TEST_ASSERT_EQUAL_HEX32(MEM_HEAD_SENTINEL_BOT, mem->botSentinel);
	TEST_ASSERT_EQUAL_HEX32(MEM_FOOT_SENTINEL, mem->footer->sentinel);
	
	Mem_Free(ptr);
}

void test_Mem_Alloc_DataIsZeroed(void) {
	memPool_t *pool;
	byte *ptr;
	int i;
	qBool allZero = qTrue;
	
	pool = Mem_CreatePool("zero_test");
	ptr = (byte *)Mem_Alloc(256, pool, 0);
	
	// Check that data is zeroed (calloc behavior)
	for (i = 0; i < 256; i++) {
		if (ptr[i] != 0) {
			allZero = qFalse;
			break;
		}
	}
	
	TEST_ASSERT_TRUE(allZero);
	Mem_Free(ptr);
}

// ============================================================================
// TESTS: Tag Management
// ============================================================================

void test_Mem_Tag_SingleTag(void) {
	memPool_t *pool;
	void *ptr1, *ptr2;
	
	pool = Mem_CreatePool("tag_test");
	ptr1 = Mem_Alloc(100, pool, 1);
	ptr2 = Mem_Alloc(200, pool, 1);
	(void)ptr1; (void)ptr2;
	
	TEST_ASSERT_EQUAL(2, pool->blockCount);
	
	Mem_FreeTag(pool, 1);
	
	TEST_ASSERT_EQUAL(0, pool->blockCount);
	TEST_ASSERT_EQUAL(0, pool->byteCount);
}

void test_Mem_Tag_MultipleTags(void) {
	memPool_t *pool;
	void *ptr1, *ptr2, *ptr3;
	
	pool = Mem_CreatePool("multi_tag");
	ptr1 = Mem_Alloc(100, pool, 1);
	ptr2 = Mem_Alloc(200, pool, 2);
	ptr3 = Mem_Alloc(300, pool, 1);
	(void)ptr1; (void)ptr3;
	
	TEST_ASSERT_EQUAL(3, pool->blockCount);
	
	// Free tag 1 (should free ptr1 and ptr3)
	Mem_FreeTag(pool, 1);
	
	TEST_ASSERT_EQUAL(1, pool->blockCount); // ptr2 remains
	
	Mem_Free(ptr2);
}

void test_Mem_Tag_ZeroTag(void) {
	memPool_t *pool;
	void *ptr1, *ptr2;
	
	pool = Mem_CreatePool("zero_tag");
	ptr1 = Mem_Alloc(100, pool, 0);
	ptr2 = Mem_Alloc(200, pool, 1);
	(void)ptr1;
	
	Mem_FreeTag(pool, 0);
	
	TEST_ASSERT_EQUAL(1, pool->blockCount); // Only ptr2 remains
	
	Mem_Free(ptr2);
}

void test_Mem_TagSize_Calculation(void) {
	memPool_t *pool;
	void *ptr1, *ptr2, *ptr3;
	size_t tagSize;
	
	pool = Mem_CreatePool("tagsize_test");
	ptr1 = Mem_Alloc(100, pool, 5);
	ptr2 = Mem_Alloc(200, pool, 5);
	ptr3 = Mem_Alloc(300, pool, 10);
	
	tagSize = Mem_TagSize(pool, 5);
	
	TEST_ASSERT(tagSize > 0);
	TEST_ASSERT_EQUAL(0, Mem_TagSize(pool, 99)); // Non-existent tag
	
	Mem_Free(ptr1);
	Mem_Free(ptr2);
	Mem_Free(ptr3);
}

void test_Mem_ChangeTag_Simple(void) {
	memPool_t *pool;
	void *ptr1, *ptr2;
	size_t changed;
	
	pool = Mem_CreatePool("change_tag");
	ptr1 = Mem_Alloc(100, pool, 1);
	ptr2 = Mem_Alloc(200, pool, 1);
	(void)ptr1; (void)ptr2;
	
	changed = Mem_ChangeTag(pool, 1, 2);
	
	TEST_ASSERT(changed > 0);
	TEST_ASSERT_EQUAL(0, Mem_TagSize(pool, 1)); // No more tag 1
	TEST_ASSERT(Mem_TagSize(pool, 2) > 0); // Now in tag 2
	
	Mem_FreeTag(pool, 2); // Should free both
	TEST_ASSERT_EQUAL(0, pool->blockCount);
}

// ============================================================================
// TESTS: Pool Operations
// ============================================================================

void test_Mem_FreePool_AllAllocations(void) {
	memPool_t *pool;
	void *ptr1, *ptr2, *ptr3;
	size_t freed;
	
	pool = Mem_CreatePool("freepool_test");
	ptr1 = Mem_Alloc(100, pool, 0);
	ptr2 = Mem_Alloc(200, pool, 1);
	ptr3 = Mem_Alloc(300, pool, 2);
	(void)ptr1; (void)ptr2; (void)ptr3;
	
	TEST_ASSERT_EQUAL(3, pool->blockCount);
	
	freed = Mem_FreePool(pool);
	
	TEST_ASSERT(freed > 0);
	TEST_ASSERT_EQUAL(0, pool->blockCount);
	TEST_ASSERT_EQUAL(0, pool->byteCount);
}

void test_Mem_PoolSize_Tracks(void) {
	memPool_t *pool;
	void *ptr1, *ptr2;
	size_t size1, size2, size3;
	
	pool = Mem_CreatePool("poolsize_test");
	size1 = Mem_PoolSize(pool);
	
	ptr1 = Mem_Alloc(100, pool, 0);
	size2 = Mem_PoolSize(pool);
	
	ptr2 = Mem_Alloc(200, pool, 0);
	size3 = Mem_PoolSize(pool);
	
	TEST_ASSERT_EQUAL(0, size1);
	TEST_ASSERT(size2 > size1);
	TEST_ASSERT(size3 > size2);
	
	Mem_Free(ptr1);
	Mem_Free(ptr2);
}

void test_Mem_DeletePool_WithAllocations(void) {
	memPool_t *pool;
	void *ptr1, *ptr2;
	size_t freed;
	
	pool = Mem_CreatePool("delete_test");
	ptr1 = Mem_Alloc(100, pool, 0);
	ptr2 = Mem_Alloc(200, pool, 0);
	(void)ptr1; (void)ptr2;
	
	TEST_ASSERT_EQUAL(2, pool->blockCount);
	
	freed = Mem_DeletePool(pool);
	
	TEST_ASSERT(freed > 0);
	TEST_ASSERT_FALSE(pool->inUse);
	TEST_ASSERT_EQUAL(0, pool->blockCount);
}

// ============================================================================
// TESTS: String Duplication
// ============================================================================

void test_Mem_StrDup_Simple(void) {
	memPool_t *pool;
	char *dup;
	
	pool = Mem_CreatePool("strdup_test");
	dup = Mem_StrDup("Hello, World!", pool, 0);
	
	TEST_ASSERT_NOT_NULL(dup);
	TEST_ASSERT_EQUAL_STRING("Hello, World!", dup);
	TEST_ASSERT_EQUAL(1, pool->blockCount);
	
	Mem_Free(dup);
}

void test_Mem_StrDup_EmptyString(void) {
	memPool_t *pool;
	char *dup;
	
	pool = Mem_CreatePool("empty_strdup");
	dup = Mem_StrDup("", pool, 0);
	
	TEST_ASSERT_NOT_NULL(dup);
	TEST_ASSERT_EQUAL_STRING("", dup);
	
	Mem_Free(dup);
}

void test_Mem_StrDup_LongString(void) {
	memPool_t *pool;
	char *dup;
	char longStr[1024];
	int i;
	
	pool = Mem_CreatePool("long_strdup");
	
	// Create a long string
	for (i = 0; i < 1023; i++) {
		longStr[i] = 'A' + (i % 26);
	}
	longStr[1023] = '\0';
	
	dup = Mem_StrDup(longStr, pool, 0);
	
	TEST_ASSERT_NOT_NULL(dup);
	TEST_ASSERT_EQUAL_STRING(longStr, dup);
	
	Mem_Free(dup);
}

// ============================================================================
// TESTS: No Leak Cycles
// ============================================================================

void test_Mem_NoLeak_AllocFreeCycle(void) {
	memPool_t *pool;
	void *ptr;
	int i;
	
	pool = Mem_CreatePool("leak_test");
	
	// Allocate and free 100 times
	for (i = 0; i < 100; i++) {
		ptr = Mem_Alloc(128, pool, 0);
		TEST_ASSERT_NOT_NULL(ptr);
		Mem_Free(ptr);
	}
	
	// Pool should be empty
	TEST_ASSERT_EQUAL(0, pool->blockCount);
	TEST_ASSERT_EQUAL(0, pool->byteCount);
}

void test_Mem_NoLeak_MultipleAllocFreeCycle(void) {
	memPool_t *pool;
	void *ptrs[10];
	int i, j;
	
	pool = Mem_CreatePool("multi_leak_test");
	
	// Do 10 cycles of allocating 10 blocks and freeing them
	for (j = 0; j < 10; j++) {
		for (i = 0; i < 10; i++) {
			ptrs[i] = Mem_Alloc(64 + i * 16, pool, 0);
		}
		TEST_ASSERT_EQUAL(10, pool->blockCount);
		
		for (i = 0; i < 10; i++) {
			Mem_Free(ptrs[i]);
		}
		TEST_ASSERT_EQUAL(0, pool->blockCount);
	}
	
	TEST_ASSERT_EQUAL(0, pool->byteCount);
}

void test_Mem_NoLeak_TagFreeCycle(void) {
	memPool_t *pool;
	int i, j;
	
	pool = Mem_CreatePool("tag_leak_test");
	
	// Allocate with tags and free by tag
	for (i = 0; i < 20; i++) {
		for (j = 0; j < 5; j++) {
			Mem_Alloc(100, pool, 1);
		}
		TEST_ASSERT_EQUAL(5, pool->blockCount);
		
		Mem_FreeTag(pool, 1);
		TEST_ASSERT_EQUAL(0, pool->blockCount);
	}
	
	TEST_ASSERT_EQUAL(0, pool->byteCount);
}

void test_Mem_NoLeak_PoolDeleteCycle(void) {
	memPool_t *pool;
	int i, j;
	
	// Create and delete pools with allocations
	for (i = 0; i < 10; i++) {
		pool = Mem_CreatePool("cycle_pool");
		
		for (j = 0; j < 5; j++) {
			Mem_Alloc(256, pool, 0);
		}
		
		TEST_ASSERT_EQUAL(5, pool->blockCount);
		Mem_DeletePool(pool);
		TEST_ASSERT_FALSE(pool->inUse);
	}
}

void test_Mem_NoLeak_MixedOperations(void) {
	memPool_t *pool;
	void *ptr1, *ptr2, *ptr3;
	int i;
	
	pool = Mem_CreatePool("mixed_ops");
	
	for (i = 0; i < 50; i++) {
		// Allocate with different tags
		ptr1 = Mem_Alloc(64, pool, 1);
		ptr2 = Mem_Alloc(128, pool, 2);
		ptr3 = Mem_Alloc(256, pool, 1);
		(void)ptr1; (void)ptr3;
		
		TEST_ASSERT_EQUAL(3, pool->blockCount);
		
		// Free one manually
		Mem_Free(ptr2);
		TEST_ASSERT_EQUAL(2, pool->blockCount);
		
		// Free the rest by tag
		Mem_FreeTag(pool, 1);
		TEST_ASSERT_EQUAL(0, pool->blockCount);
	}
	
	TEST_ASSERT_EQUAL(0, pool->byteCount);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

void test_Mem_Alloc_VaryingSizes(void) {
	memPool_t *pool;
	void *ptr1, *ptr2, *ptr3, *ptr4;
	
	pool = Mem_CreatePool("varying_sizes");
	
	ptr1 = Mem_Alloc(1, pool, 0);     // Tiny
	ptr2 = Mem_Alloc(100, pool, 0);   // Small
	ptr3 = Mem_Alloc(10000, pool, 0); // Medium
	ptr4 = Mem_Alloc(100000, pool, 0);// Large
	
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_NOT_NULL(ptr2);
	TEST_ASSERT_NOT_NULL(ptr3);
	TEST_ASSERT_NOT_NULL(ptr4);
	TEST_ASSERT_EQUAL(4, pool->blockCount);
	
	Mem_Free(ptr1);
	Mem_Free(ptr2);
	Mem_Free(ptr3);
	Mem_Free(ptr4);
}

void test_Mem_Alloc_AlignedAllocations(void) {
	memPool_t *pool;
	void *ptr;
	memBlock_t *mem;
	size_t addr;
	
	pool = Mem_CreatePool("aligned_test");
	ptr = Mem_Alloc(100, pool, 0);
	
	// Check that internal allocation is aligned (at memBlock_t level)
	mem = (memBlock_t *)((byte *)ptr - sizeof(memBlock_t));
	addr = (size_t)mem;
	TEST_ASSERT_EQUAL(0, addr % 32); // memBlock_t should be 32-byte aligned
	
	Mem_Free(ptr);
}

void test_Mem_FreePool_EmptyPool(void) {
	memPool_t *pool;
	size_t freed;
	
	pool = Mem_CreatePool("empty_freepool");
	freed = Mem_FreePool(pool);
	
	TEST_ASSERT_EQUAL(0, freed);
	TEST_ASSERT_EQUAL(0, pool->blockCount);
}

void test_Mem_ChangeTag_NoMatches(void) {
	memPool_t *pool;
	void *ptr;
	size_t changed;
	
	pool = Mem_CreatePool("change_nomatch");
	ptr = Mem_Alloc(100, pool, 1);
	
	changed = Mem_ChangeTag(pool, 99, 100); // Tag 99 doesn't exist
	
	TEST_ASSERT_EQUAL(0, changed);
	TEST_ASSERT(Mem_TagSize(pool, 1) > 0); // Original tag unchanged
	
	Mem_Free(ptr);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
	// Initialize memory system
	ResetMemoryState();
	
	UNITY_BEGIN();

	// Pool management tests
	RUN_TEST(test_Mem_CreatePool_Basic);
	RUN_TEST(test_Mem_CreatePool_Multiple);
	RUN_TEST(test_Mem_CreatePool_ReturnsExisting);
	RUN_TEST(test_Mem_DeletePool_EmptyPool);

	// Basic allocation and freeing tests
	RUN_TEST(test_Mem_Alloc_SimpleAllocation);
	RUN_TEST(test_Mem_Alloc_ZeroSize);
	RUN_TEST(test_Mem_Alloc_MultipleAllocations);
	RUN_TEST(test_Mem_Free_DecrementsCounts);
	RUN_TEST(test_Mem_Alloc_Free_NullPointer);

	// Memory integrity tests
	RUN_TEST(test_Mem_Alloc_SetsSentinels);
	RUN_TEST(test_Mem_Alloc_DataIsZeroed);

	// Tag management tests
	RUN_TEST(test_Mem_Tag_SingleTag);
	RUN_TEST(test_Mem_Tag_MultipleTags);
	RUN_TEST(test_Mem_Tag_ZeroTag);
	RUN_TEST(test_Mem_TagSize_Calculation);
	RUN_TEST(test_Mem_ChangeTag_Simple);

	// Pool operations tests
	RUN_TEST(test_Mem_FreePool_AllAllocations);
	RUN_TEST(test_Mem_PoolSize_Tracks);
	RUN_TEST(test_Mem_DeletePool_WithAllocations);

	// String duplication tests
	RUN_TEST(test_Mem_StrDup_Simple);
	RUN_TEST(test_Mem_StrDup_EmptyString);
	RUN_TEST(test_Mem_StrDup_LongString);

	// No leak cycle tests
	RUN_TEST(test_Mem_NoLeak_AllocFreeCycle);
	RUN_TEST(test_Mem_NoLeak_MultipleAllocFreeCycle);
	RUN_TEST(test_Mem_NoLeak_TagFreeCycle);
	RUN_TEST(test_Mem_NoLeak_PoolDeleteCycle);
	RUN_TEST(test_Mem_NoLeak_MixedOperations);

	// Edge cases
	RUN_TEST(test_Mem_Alloc_VaryingSizes);
	RUN_TEST(test_Mem_Alloc_AlignedAllocations);
	RUN_TEST(test_Mem_FreePool_EmptyPool);
	RUN_TEST(test_Mem_ChangeTag_NoMatches);

	return UNITY_END();
}
