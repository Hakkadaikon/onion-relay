#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <vector>

extern "C" {

// ============================================================================
// Type redefinitions (avoid _Static_assert in C++ compilation)
// ============================================================================

typedef uint32_t page_id_t;
#define PAGE_ID_NULL ((page_id_t)0)
#define DB_PAGE_SIZE 4096

typedef struct {
  uint8_t data[DB_PAGE_SIZE];
} __attribute__((aligned(DB_PAGE_SIZE))) PageData;

typedef struct {
  int32_t   fd;
  uint32_t  total_pages;
  page_id_t free_list_head;
  uint32_t  free_page_count;
  char      path[256];
} DiskManager;

#define BUFFER_FRAME_INVALID ((uint32_t)0xFFFFFFFF)

typedef struct {
  page_id_t*   page_ids;
  uint8_t*     pin_counts;
  uint8_t*     dirty_flags;
  uint8_t*     ref_bits;
  PageData*    pages;
  uint64_t*    lsn;
  uint32_t     pool_size;
  uint32_t     clock_hand;
  uint32_t*    hash_table;
  uint32_t     hash_size;
  DiskManager* disk;
} BufferPool;

typedef enum {
  NOSTR_DB_OK                     = 0,
  NOSTR_DB_ERROR_FILE_OPEN        = -1,
  NOSTR_DB_ERROR_FILE_CREATE      = -2,
  NOSTR_DB_ERROR_MMAP_FAILED      = -3,
  NOSTR_DB_ERROR_INVALID_MAGIC    = -4,
  NOSTR_DB_ERROR_VERSION_MISMATCH = -5,
  NOSTR_DB_ERROR_FULL             = -6,
  NOSTR_DB_ERROR_NOT_FOUND        = -7,
  NOSTR_DB_ERROR_DUPLICATE        = -8,
  NOSTR_DB_ERROR_INVALID_EVENT    = -9,
  NOSTR_DB_ERROR_INDEX_CORRUPT    = -10,
  NOSTR_DB_ERROR_NULL_PARAM       = -11,
  NOSTR_DB_ERROR_FSTAT_FAILED     = -12,
  NOSTR_DB_ERROR_FTRUNCATE_FAILED = -13,
} NostrDBError;

typedef enum {
  PAGE_TYPE_FREE        = 0,
  PAGE_TYPE_FILE_HEADER = 1,
  PAGE_TYPE_RECORD      = 2,
  PAGE_TYPE_BTREE_LEAF  = 3,
  PAGE_TYPE_BTREE_INNER = 4,
  PAGE_TYPE_OVERFLOW    = 5,
} PageType;

typedef struct {
  page_id_t page_id;
  page_id_t overflow_page;
  uint16_t  slot_count;
  uint16_t  free_space_start;
  uint16_t  free_space_end;
  uint16_t  fragmented_space;
  uint8_t   page_type;
  uint8_t   flags;
  uint8_t   reserved[6];
} SlotPageHeader;

#define SLOT_PAGE_HEADER_SIZE sizeof(SlotPageHeader)
#define SLOT_ENTRY_SIZE sizeof(SlotEntry)
#define SLOT_PAGE_DATA_SPACE (DB_PAGE_SIZE - SLOT_PAGE_HEADER_SIZE)
#define SPANNED_MARKER ((uint16_t)0xFFFF)

typedef struct {
  uint16_t offset;
  uint16_t length;
} SlotEntry;

typedef struct {
  page_id_t page_id;
  uint16_t  slot_index;
} RecordId;

#define RECORD_ID_NULL \
  (RecordId) { PAGE_ID_NULL, 0 }

typedef int32_t (*BTreeKeyCompare)(const void* a, const void* b,
                                   uint16_t key_size);

typedef enum {
  BTREE_KEY_BYTES32       = 0,
  BTREE_KEY_INT64         = 1,
  BTREE_KEY_UINT32        = 2,
  BTREE_KEY_COMPOSITE     = 3,
  BTREE_KEY_COMPOSITE_TAG = 4,
} BTreeKeyType;

typedef struct {
  page_id_t root_page;
  uint32_t  height;
  uint32_t  entry_count;
  uint32_t  leaf_count;
  uint32_t  inner_count;
  uint16_t  key_size;
  uint16_t  value_size;
  uint8_t   key_type;
  uint8_t   flags;
  uint8_t   reserved[10];
} BTreeMeta;

typedef struct {
  uint16_t  key_count;
  uint8_t   is_leaf;
  uint8_t   reserved;
  page_id_t right_sibling;
} BTreeNodeHeader;

#define BTREE_NODE_HEADER_OFFSET SLOT_PAGE_HEADER_SIZE
#define BTREE_DATA_OFFSET (SLOT_PAGE_HEADER_SIZE + sizeof(BTreeNodeHeader))
#define BTREE_NODE_SPACE (DB_PAGE_SIZE - BTREE_DATA_OFFSET)

#define BTREE_LEAF_MAX_KEYS(ks, vs) \
  ((uint16_t)(BTREE_NODE_SPACE / ((ks) + (vs))))

#define BTREE_INNER_MAX_KEYS(ks) \
  ((uint16_t)((BTREE_NODE_SPACE - sizeof(page_id_t)) / \
              ((ks) + sizeof(page_id_t))))

typedef struct {
  BufferPool*     pool;
  page_id_t       meta_page;
  BTreeMeta       meta;
  BTreeKeyCompare compare;
} BTree;

typedef bool (*BTreeScanCallback)(const void* key, const void* value,
                                  void* user_data);

#define BTREE_MAX_HEIGHT 16

typedef struct {
  page_id_t page_ids[BTREE_MAX_HEIGHT];
  uint16_t  positions[BTREE_MAX_HEIGHT];
  uint16_t  depth;
} BTreePath;

typedef struct {
  page_id_t next_page;
  uint16_t  entry_count;
  uint16_t  reserved;
} BTreeOverflowHeader;

#define BTREE_OVERFLOW_ENTRY_SIZE sizeof(RecordId)
#define BTREE_OVERFLOW_MAX_ENTRIES \
  ((DB_PAGE_SIZE - sizeof(BTreeOverflowHeader)) / BTREE_OVERFLOW_ENTRY_SIZE)

typedef struct {
  uint8_t  id[32];
  uint8_t  pubkey[32];
  uint8_t  sig[64];
  int64_t  created_at;
  uint32_t kind;
  uint32_t flags;
  uint16_t content_length;
  uint16_t tags_length;
} EventRecord;

typedef struct {
  page_id_t next_page;
  uint16_t  data_length;
  uint16_t  reserved;
} OverflowHeader;

#define OVERFLOW_DATA_SPACE (DB_PAGE_SIZE - sizeof(OverflowHeader))

#define NOSTR_DB_EVENT_FLAG_DELETED (1 << 0)

// IndexManager
typedef struct {
  BTree       id_index;
  BTree       timeline_index;
  BTree       pubkey_index;
  BTree       kind_index;
  BTree       pubkey_kind_index;
  BTree       tag_index;
  BufferPool* pool;
} IndexManager;

// Query types
#define NOSTR_DB_FILTER_MAX_IDS 256
#define NOSTR_DB_FILTER_MAX_AUTHORS 256
#define NOSTR_DB_FILTER_MAX_KINDS 64
#define NOSTR_DB_FILTER_MAX_TAGS 26
#define NOSTR_DB_FILTER_MAX_TAG_VALUES 256
#define NOSTR_DB_RESULT_DEFAULT_CAPACITY 100
#define NOSTR_DB_QUERY_DEFAULT_LIMIT 500

typedef struct {
  uint8_t value[32];
  size_t  prefix_len;
} NostrDBFilterId;

typedef struct {
  uint8_t value[32];
  size_t  prefix_len;
} NostrDBFilterPubkey;

typedef struct {
  char    name;
  uint8_t values[NOSTR_DB_FILTER_MAX_TAG_VALUES][32];
  size_t  values_count;
} NostrDBFilterTag;

typedef struct {
  NostrDBFilterId     ids[NOSTR_DB_FILTER_MAX_IDS];
  size_t              ids_count;
  NostrDBFilterPubkey authors[NOSTR_DB_FILTER_MAX_AUTHORS];
  size_t              authors_count;
  uint32_t            kinds[NOSTR_DB_FILTER_MAX_KINDS];
  size_t              kinds_count;
  NostrDBFilterTag    tags[NOSTR_DB_FILTER_MAX_TAGS];
  size_t              tags_count;
  int64_t             since;
  int64_t             until;
  uint32_t            limit;
} NostrDBFilter;

typedef struct {
  RecordId* rids;
  int64_t*  created_at;
  uint32_t  count;
  uint32_t  capacity;
  uint64_t  bloom[64];
} QueryResultSet;

typedef enum {
  NOSTR_DB_QUERY_STRATEGY_BY_ID = 0,
  NOSTR_DB_QUERY_STRATEGY_BY_PUBKEY_KIND,
  NOSTR_DB_QUERY_STRATEGY_BY_PUBKEY,
  NOSTR_DB_QUERY_STRATEGY_BY_KIND,
  NOSTR_DB_QUERY_STRATEGY_BY_TAG,
  NOSTR_DB_QUERY_STRATEGY_TIMELINE_SCAN,
} NostrDBQueryStrategy;

// Disk/Buffer/B+tree API
NostrDBError disk_manager_create(DiskManager* dm, const char* path,
                                 uint32_t initial_pages);
void         disk_manager_close(DiskManager* dm);
NostrDBError disk_free_page(DiskManager* dm, page_id_t page_id);
NostrDBError buffer_pool_init(BufferPool* pool, DiskManager* disk,
                              uint32_t pool_size);
void         buffer_pool_shutdown(BufferPool* pool);
PageData*    buffer_pool_pin(BufferPool* pool, page_id_t page_id);
page_id_t    buffer_pool_alloc_page(BufferPool* pool, PageData** out_page);
void         buffer_pool_unpin(BufferPool* pool, page_id_t page_id);
void buffer_pool_mark_dirty(BufferPool* pool, page_id_t page_id, uint64_t lsn);
NostrDBError buffer_pool_flush_all(BufferPool* pool);

// Record manager API
NostrDBError record_insert(BufferPool* pool, const void* data, uint16_t length,
                           RecordId* out_rid);
NostrDBError record_read(BufferPool* pool, RecordId rid, void* out,
                         uint16_t* length);
NostrDBError record_delete(BufferPool* pool, RecordId rid);

// Index manager API
NostrDBError index_manager_create(IndexManager* im, BufferPool* pool);
NostrDBError index_manager_insert_event(IndexManager* im, RecordId rid,
                                        const EventRecord* record,
                                        const uint8_t*     tags_data,
                                        uint16_t           tags_length);
NostrDBError index_manager_delete_event(IndexManager* im, RecordId rid,
                                        const EventRecord* record,
                                        const uint8_t*     tags_data,
                                        uint16_t           tags_length);

// Query engine API
QueryResultSet* query_result_create(uint32_t capacity);
void            query_result_free(QueryResultSet* rs);
int32_t         query_result_add(QueryResultSet* rs, RecordId rid,
                                 int64_t created_at);
int32_t         query_result_sort(QueryResultSet* rs);
void            query_result_apply_limit(QueryResultSet* rs, uint32_t limit);

NostrDBError query_execute(IndexManager* im, BufferPool* pool,
                           const NostrDBFilter* filter, QueryResultSet* rs);
NostrDBError query_by_ids(IndexManager* im, BufferPool* pool,
                          const NostrDBFilter* filter, QueryResultSet* rs);
NostrDBError query_by_pubkey(IndexManager* im, const NostrDBFilter* filter,
                             QueryResultSet* rs);
NostrDBError query_by_kind(IndexManager* im, const NostrDBFilter* filter,
                           QueryResultSet* rs);
NostrDBError query_by_pubkey_kind(IndexManager* im,
                                  const NostrDBFilter* filter,
                                  QueryResultSet*      rs);
NostrDBError query_by_tag(IndexManager* im, const NostrDBFilter* filter,
                          QueryResultSet* rs);
NostrDBError query_timeline_scan(IndexManager* im, const NostrDBFilter* filter,
                                 QueryResultSet* rs);
NostrDBError query_post_filter(BufferPool* pool, QueryResultSet* rs,
                               const NostrDBFilter* filter);

}  // extern "C"

// ============================================================================
// Test fixture
// ============================================================================
class QueryEngineTest : public ::testing::Test {
 protected:
  DiskManager  dm;
  BufferPool   pool;
  IndexManager im;
  char         path[256];

  void SetUp() override {
    snprintf(path, sizeof(path), "/tmp/nostr_query_test_%d.dat", getpid());
    ASSERT_EQ(NOSTR_DB_OK, disk_manager_create(&dm, path, 8192));
    ASSERT_EQ(NOSTR_DB_OK, buffer_pool_init(&pool, &dm, 1024));
    ASSERT_EQ(NOSTR_DB_OK, index_manager_create(&im, &pool));
  }

  void TearDown() override {
    buffer_pool_shutdown(&pool);
    disk_manager_close(&dm);
    unlink(path);
  }

  // Helper: Create and insert an event, returning its RecordId
  RecordId insert_event(uint8_t id_byte, uint8_t pk_byte, int64_t created_at,
                        uint32_t kind) {
    EventRecord rec;
    memset(&rec, 0, sizeof(rec));
    memset(rec.id, id_byte, 32);
    memset(rec.pubkey, pk_byte, 32);
    rec.created_at     = created_at;
    rec.kind           = kind;
    rec.content_length = 0;
    rec.tags_length    = 0;

    RecordId rid;
    EXPECT_EQ(NOSTR_DB_OK,
              record_insert(&pool, &rec, sizeof(EventRecord), &rid));
    EXPECT_EQ(NOSTR_DB_OK,
              index_manager_insert_event(&im, rid, &rec, nullptr, 0));
    return rid;
  }
};

// ============================================================================
// Result set tests
// ============================================================================
TEST(QueryResultTest, CreateAndFree) {
  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);
  EXPECT_EQ(0u, rs->count);
  EXPECT_GE(rs->capacity, (uint32_t)NOSTR_DB_RESULT_DEFAULT_CAPACITY);
  query_result_free(rs);
}

TEST(QueryResultTest, AddAndDedup) {
  QueryResultSet* rs = query_result_create(10);
  ASSERT_NE(nullptr, rs);

  RecordId r1 = {10, 0};
  RecordId r2 = {20, 1};

  EXPECT_EQ(0, query_result_add(rs, r1, 1000));
  EXPECT_EQ(0, query_result_add(rs, r2, 2000));
  EXPECT_EQ(2u, rs->count);

  // Duplicate should be detected
  EXPECT_EQ(1, query_result_add(rs, r1, 1000));
  EXPECT_EQ(2u, rs->count);

  query_result_free(rs);
}

TEST(QueryResultTest, SortDescending) {
  QueryResultSet* rs = query_result_create(10);
  ASSERT_NE(nullptr, rs);

  RecordId r1 = {1, 0}, r2 = {2, 0}, r3 = {3, 0};
  query_result_add(rs, r1, 1000);
  query_result_add(rs, r2, 3000);
  query_result_add(rs, r3, 2000);

  query_result_sort(rs);
  EXPECT_EQ(3000, rs->created_at[0]);
  EXPECT_EQ(2000, rs->created_at[1]);
  EXPECT_EQ(1000, rs->created_at[2]);

  query_result_free(rs);
}

TEST(QueryResultTest, ApplyLimit) {
  QueryResultSet* rs = query_result_create(10);
  ASSERT_NE(nullptr, rs);

  for (int i = 0; i < 5; i++) {
    RecordId r = {(page_id_t)(i + 1), 0};
    query_result_add(rs, r, (int64_t)(i * 100));
  }
  EXPECT_EQ(5u, rs->count);

  query_result_apply_limit(rs, 3);
  EXPECT_EQ(3u, rs->count);

  query_result_free(rs);
}

// ============================================================================
// Query by ID tests
// ============================================================================
TEST_F(QueryEngineTest, QueryByIds) {
  RecordId r1 = insert_event(0x01, 0xAA, 1000, 1);
  RecordId r2 = insert_event(0x02, 0xBB, 2000, 1);
  insert_event(0x03, 0xCC, 3000, 2);

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  memset(filter.ids[0].value, 0x01, 32);
  memset(filter.ids[1].value, 0x02, 32);
  filter.ids_count = 2;

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_by_ids(&im, &pool, &filter, rs));
  EXPECT_EQ(2u, rs->count);

  query_result_free(rs);
}

TEST_F(QueryEngineTest, QueryByIdsWithTimeFilter) {
  insert_event(0x01, 0xAA, 1000, 1);
  insert_event(0x02, 0xBB, 2000, 1);
  insert_event(0x03, 0xCC, 3000, 1);

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  memset(filter.ids[0].value, 0x01, 32);
  memset(filter.ids[1].value, 0x02, 32);
  memset(filter.ids[2].value, 0x03, 32);
  filter.ids_count = 3;
  filter.since     = 1500;
  filter.until     = 2500;

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_by_ids(&im, &pool, &filter, rs));
  EXPECT_EQ(1u, rs->count);  // Only event at 2000

  query_result_free(rs);
}

// ============================================================================
// Query by kind tests
// ============================================================================
TEST_F(QueryEngineTest, QueryByKind) {
  insert_event(0x01, 0xAA, 1000, 1);
  insert_event(0x02, 0xBB, 2000, 1);
  insert_event(0x03, 0xCC, 3000, 2);

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  filter.kinds[0]   = 1;
  filter.kinds_count = 1;

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_by_kind(&im, &filter, rs));
  EXPECT_EQ(2u, rs->count);

  query_result_free(rs);
}

// ============================================================================
// Query by pubkey tests
// ============================================================================
TEST_F(QueryEngineTest, QueryByPubkey) {
  insert_event(0x01, 0xAA, 1000, 1);
  insert_event(0x02, 0xAA, 2000, 2);
  insert_event(0x03, 0xBB, 3000, 1);

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  memset(filter.authors[0].value, 0xAA, 32);
  filter.authors_count = 1;

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_by_pubkey(&im, &filter, rs));
  EXPECT_EQ(2u, rs->count);

  query_result_free(rs);
}

// ============================================================================
// Query by pubkey+kind tests
// ============================================================================
TEST_F(QueryEngineTest, QueryByPubkeyKind) {
  insert_event(0x01, 0xAA, 1000, 1);
  insert_event(0x02, 0xAA, 2000, 2);
  insert_event(0x03, 0xAA, 3000, 1);
  insert_event(0x04, 0xBB, 4000, 1);

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  memset(filter.authors[0].value, 0xAA, 32);
  filter.authors_count = 1;
  filter.kinds[0]      = 1;
  filter.kinds_count   = 1;

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_by_pubkey_kind(&im, &filter, rs));
  EXPECT_EQ(2u, rs->count);

  query_result_free(rs);
}

// ============================================================================
// Timeline scan tests
// ============================================================================
TEST_F(QueryEngineTest, TimelineScan) {
  insert_event(0x01, 0xAA, 1000, 1);
  insert_event(0x02, 0xBB, 2000, 1);
  insert_event(0x03, 0xCC, 3000, 2);

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_timeline_scan(&im, &filter, rs));
  EXPECT_EQ(3u, rs->count);

  query_result_free(rs);
}

TEST_F(QueryEngineTest, TimelineScanWithRange) {
  insert_event(0x01, 0xAA, 1000, 1);
  insert_event(0x02, 0xBB, 2000, 1);
  insert_event(0x03, 0xCC, 3000, 2);
  insert_event(0x04, 0xDD, 4000, 2);

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  filter.since = 1500;
  filter.until = 3500;

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_timeline_scan(&im, &filter, rs));
  EXPECT_EQ(2u, rs->count);  // events at 2000 and 3000

  query_result_free(rs);
}

// ============================================================================
// query_execute integration tests
// ============================================================================
TEST_F(QueryEngineTest, ExecuteSelectsIdStrategy) {
  insert_event(0x01, 0xAA, 1000, 1);
  insert_event(0x02, 0xBB, 2000, 1);

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  memset(filter.ids[0].value, 0x01, 32);
  filter.ids_count = 1;

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_execute(&im, &pool, &filter, rs));
  EXPECT_EQ(1u, rs->count);

  query_result_free(rs);
}

TEST_F(QueryEngineTest, ExecuteWithLimit) {
  for (int i = 0; i < 10; i++) {
    insert_event((uint8_t)i, 0xAA, (int64_t)(1000 + i * 100), 1);
  }

  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  filter.limit = 3;
  // No specific filter -> timeline scan

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_execute(&im, &pool, &filter, rs));
  EXPECT_LE(rs->count, 3u);

  // Results should be sorted descending
  if (rs->count > 1) {
    EXPECT_GE(rs->created_at[0], rs->created_at[1]);
  }

  query_result_free(rs);
}

TEST_F(QueryEngineTest, ExecuteEmptyResult) {
  // No events inserted
  NostrDBFilter filter;
  memset(&filter, 0, sizeof(filter));
  memset(filter.ids[0].value, 0xFF, 32);
  filter.ids_count = 1;

  QueryResultSet* rs = query_result_create(0);
  ASSERT_NE(nullptr, rs);

  ASSERT_EQ(NOSTR_DB_OK, query_execute(&im, &pool, &filter, rs));
  EXPECT_EQ(0u, rs->count);

  query_result_free(rs);
}
