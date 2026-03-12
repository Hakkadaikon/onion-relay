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

// Event record header
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

// Disk manager API
NostrDBError disk_manager_create(DiskManager* dm, const char* path,
                                 uint32_t initial_pages);
void         disk_manager_close(DiskManager* dm);
NostrDBError disk_free_page(DiskManager* dm, page_id_t page_id);

// Buffer pool API
NostrDBError buffer_pool_init(BufferPool* pool, DiskManager* disk,
                              uint32_t pool_size);
void         buffer_pool_shutdown(BufferPool* pool);
PageData*    buffer_pool_pin(BufferPool* pool, page_id_t page_id);
page_id_t    buffer_pool_alloc_page(BufferPool* pool, PageData** out_page);
void         buffer_pool_unpin(BufferPool* pool, page_id_t page_id);
void buffer_pool_mark_dirty(BufferPool* pool, page_id_t page_id, uint64_t lsn);
NostrDBError buffer_pool_flush(BufferPool* pool, page_id_t page_id);
NostrDBError buffer_pool_flush_all(BufferPool* pool);

// B+ tree API
NostrDBError btree_create(BTree* tree, BufferPool* pool, uint16_t key_size,
                          uint16_t value_size, BTreeKeyType key_type);
NostrDBError btree_open(BTree* tree, BufferPool* pool, page_id_t meta_page);
NostrDBError btree_flush_meta(BTree* tree);
NostrDBError btree_insert(BTree* tree, const void* key, const void* value);
NostrDBError btree_search(BTree* tree, const void* key, void* value_out);
NostrDBError btree_delete(BTree* tree, const void* key);
NostrDBError btree_range_scan(BTree* tree, const void* min_key,
                              const void*       max_key,
                              BTreeScanCallback callback, void* user_data);
NostrDBError btree_insert_dup(BTree* tree, const void* key, RecordId rid);
NostrDBError btree_scan_key(BTree* tree, const void* key,
                            BTreeScanCallback callback, void* user_data);
NostrDBError btree_delete_dup(BTree* tree, const void* key, RecordId rid);

// Index API
NostrDBError index_manager_create(IndexManager* im, BufferPool* pool);
NostrDBError index_manager_open(IndexManager* im, BufferPool* pool,
                                page_id_t id_meta, page_id_t timeline_meta,
                                page_id_t pubkey_meta, page_id_t kind_meta,
                                page_id_t pk_kind_meta, page_id_t tag_meta);
NostrDBError index_manager_flush(IndexManager* im);
NostrDBError index_manager_insert_event(IndexManager* im, RecordId rid,
                                        const EventRecord* record,
                                        const uint8_t*     tags_data,
                                        uint16_t           tags_length);
NostrDBError index_manager_delete_event(IndexManager* im, RecordId rid,
                                        const EventRecord* record,
                                        const uint8_t*     tags_data,
                                        uint16_t           tags_length);

// Individual index operations
NostrDBError index_id_insert(BTree* tree, const uint8_t id[32], RecordId rid);
NostrDBError index_id_lookup(BTree* tree, const uint8_t id[32],
                             RecordId* out_rid);
NostrDBError index_id_delete(BTree* tree, const uint8_t id[32]);

NostrDBError index_timeline_insert(BTree* tree, int64_t created_at,
                                   RecordId rid);
NostrDBError index_timeline_delete(BTree* tree, int64_t created_at,
                                   RecordId rid);

NostrDBError index_pubkey_insert(BTree* tree, const uint8_t pubkey[32],
                                 RecordId rid);
NostrDBError index_pubkey_delete(BTree* tree, const uint8_t pubkey[32],
                                 RecordId rid);

NostrDBError index_kind_insert(BTree* tree, uint32_t kind, RecordId rid);
NostrDBError index_kind_delete(BTree* tree, uint32_t kind, RecordId rid);

NostrDBError index_pk_kind_insert(BTree* tree, const uint8_t pubkey[32],
                                  uint32_t kind, RecordId rid);
NostrDBError index_pk_kind_delete(BTree* tree, const uint8_t pubkey[32],
                                  uint32_t kind, RecordId rid);

NostrDBError index_tag_insert(BTree* tree, uint8_t tag_name,
                              const uint8_t tag_value[32], RecordId rid);
NostrDBError index_tag_delete(BTree* tree, uint8_t tag_name,
                              const uint8_t tag_value[32], RecordId rid);

}  // extern "C"

// ============================================================================
// Test fixture
// ============================================================================
class IndexTest : public ::testing::Test {
 protected:
  DiskManager  dm;
  BufferPool   pool;
  IndexManager im;
  char         path[256];

  void SetUp() override {
    snprintf(path, sizeof(path), "/tmp/nostr_index_test_%d.dat", getpid());
    ASSERT_EQ(NOSTR_DB_OK, disk_manager_create(&dm, path, 8192));
    ASSERT_EQ(NOSTR_DB_OK, buffer_pool_init(&pool, &dm, 1024));
  }

  void TearDown() override {
    buffer_pool_shutdown(&pool);
    disk_manager_close(&dm);
    unlink(path);
  }

  // Helper: build a test EventRecord with given fields
  void make_event(EventRecord* rec, uint8_t id_byte, uint8_t pubkey_byte,
                  int64_t created_at, uint32_t kind) {
    memset(rec, 0, sizeof(EventRecord));
    memset(rec->id, id_byte, 32);
    memset(rec->pubkey, pubkey_byte, 32);
    rec->created_at     = created_at;
    rec->kind           = kind;
    rec->content_length = 0;
    rec->tags_length    = 0;
  }

  // Helper: build serialized tag data with a single tag
  // tag_name: single char, tag_value_hex: 64-char hex string
  std::vector<uint8_t> make_tags(uint8_t tag_name,
                                 const char* tag_value_hex) {
    std::vector<uint8_t> buf;
    // tag_count = 1 (little endian)
    buf.push_back(1);
    buf.push_back(0);
    // value_count = 1
    buf.push_back(1);
    // name_len = 1
    buf.push_back(1);
    // name
    buf.push_back(tag_name);
    // value_len (little endian)
    uint16_t vlen = (uint16_t)strlen(tag_value_hex);
    buf.push_back((uint8_t)(vlen & 0xFF));
    buf.push_back((uint8_t)((vlen >> 8) & 0xFF));
    // value
    for (size_t i = 0; i < vlen; i++) {
      buf.push_back((uint8_t)tag_value_hex[i]);
    }
    return buf;
  }
};

// ============================================================================
// ID index tests
// ============================================================================
TEST_F(IndexTest, IdInsertAndLookup) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 32, sizeof(RecordId), BTREE_KEY_BYTES32));

  uint8_t  id[32];
  memset(id, 0xAA, 32);
  RecordId rid = {10, 5};

  ASSERT_EQ(NOSTR_DB_OK, index_id_insert(&tree, id, rid));

  RecordId found;
  ASSERT_EQ(NOSTR_DB_OK, index_id_lookup(&tree, id, &found));
  EXPECT_EQ(found.page_id, 10u);
  EXPECT_EQ(found.slot_index, 5u);
}

TEST_F(IndexTest, IdDeleteAndNotFound) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 32, sizeof(RecordId), BTREE_KEY_BYTES32));

  uint8_t  id[32];
  memset(id, 0xBB, 32);
  RecordId rid = {20, 3};

  ASSERT_EQ(NOSTR_DB_OK, index_id_insert(&tree, id, rid));
  ASSERT_EQ(NOSTR_DB_OK, index_id_delete(&tree, id));

  RecordId found;
  EXPECT_EQ(NOSTR_DB_ERROR_NOT_FOUND, index_id_lookup(&tree, id, &found));
}

TEST_F(IndexTest, IdDuplicateReject) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 32, sizeof(RecordId), BTREE_KEY_BYTES32));

  uint8_t  id[32];
  memset(id, 0xCC, 32);
  RecordId rid1 = {10, 1};
  RecordId rid2 = {20, 2};

  ASSERT_EQ(NOSTR_DB_OK, index_id_insert(&tree, id, rid1));
  EXPECT_EQ(NOSTR_DB_ERROR_DUPLICATE, index_id_insert(&tree, id, rid2));
}

// ============================================================================
// Timeline index tests
// ============================================================================
TEST_F(IndexTest, TimelineInsertAndScan) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, sizeof(int64_t), sizeof(page_id_t),
                         BTREE_KEY_INT64));

  RecordId rid1 = {10, 0};
  RecordId rid2 = {20, 0};
  RecordId rid3 = {30, 0};

  // Insert timestamps: 1000, 2000, 3000
  ASSERT_EQ(NOSTR_DB_OK, index_timeline_insert(&tree, 1000, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_timeline_insert(&tree, 2000, rid2));
  ASSERT_EQ(NOSTR_DB_OK, index_timeline_insert(&tree, 3000, rid3));

  // Scan all — should be in descending order (3000, 2000, 1000)
  // because keys are INT64_MAX - created_at (ascending = descending time)
  struct ScanResult {
    std::vector<int64_t> keys;
  };
  ScanResult result;

  auto cb = [](const void* key, const void* /*value*/,
               void* user_data) -> bool {
    auto* r     = static_cast<ScanResult*>(user_data);
    int64_t k;
    memcpy(&k, key, sizeof(int64_t));
    r->keys.push_back(INT64_MAX - k);  // decode
    return true;
  };

  ASSERT_EQ(NOSTR_DB_OK, btree_range_scan(&tree, nullptr, nullptr, cb, &result));
  ASSERT_EQ(3u, result.keys.size());
  EXPECT_EQ(3000, result.keys[0]);
  EXPECT_EQ(2000, result.keys[1]);
  EXPECT_EQ(1000, result.keys[2]);
}

TEST_F(IndexTest, TimelineDelete) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, sizeof(int64_t), sizeof(page_id_t),
                         BTREE_KEY_INT64));

  RecordId rid1 = {10, 0};
  RecordId rid2 = {20, 0};

  ASSERT_EQ(NOSTR_DB_OK, index_timeline_insert(&tree, 1000, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_timeline_insert(&tree, 2000, rid2));
  ASSERT_EQ(NOSTR_DB_OK, index_timeline_delete(&tree, 1000, rid1));

  // Only rid2 should remain
  int64_t encoded = INT64_MAX - (int64_t)1000;
  page_id_t val   = PAGE_ID_NULL;
  EXPECT_EQ(NOSTR_DB_ERROR_NOT_FOUND, btree_search(&tree, &encoded, &val));
}

// ============================================================================
// Pubkey index tests
// ============================================================================
TEST_F(IndexTest, PubkeyInsertAndScan) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 32, sizeof(page_id_t),
                         BTREE_KEY_BYTES32));

  uint8_t pk[32];
  memset(pk, 0x11, 32);
  RecordId rid1 = {10, 0};
  RecordId rid2 = {20, 1};

  ASSERT_EQ(NOSTR_DB_OK, index_pubkey_insert(&tree, pk, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_pubkey_insert(&tree, pk, rid2));

  // Scan should find 2 entries
  struct ScanCtx {
    int count;
  };
  ScanCtx ctx = {0};

  auto cb = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };

  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&tree, pk, cb, &ctx));
  EXPECT_EQ(2, ctx.count);
}

TEST_F(IndexTest, PubkeyDelete) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 32, sizeof(page_id_t),
                         BTREE_KEY_BYTES32));

  uint8_t pk[32];
  memset(pk, 0x22, 32);
  RecordId rid1 = {10, 0};

  ASSERT_EQ(NOSTR_DB_OK, index_pubkey_insert(&tree, pk, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_pubkey_delete(&tree, pk, rid1));

  // Key should be gone (overflow page freed, key removed from B+ tree)
  page_id_t val = PAGE_ID_NULL;
  EXPECT_EQ(NOSTR_DB_ERROR_NOT_FOUND, btree_search(&tree, pk, &val));
}

// ============================================================================
// Kind index tests
// ============================================================================
TEST_F(IndexTest, KindInsertAndScan) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, sizeof(uint32_t), sizeof(page_id_t),
                         BTREE_KEY_UINT32));

  uint32_t kind = 1;
  RecordId rid1 = {10, 0};
  RecordId rid2 = {20, 1};
  RecordId rid3 = {30, 2};

  ASSERT_EQ(NOSTR_DB_OK, index_kind_insert(&tree, kind, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_kind_insert(&tree, kind, rid2));
  ASSERT_EQ(NOSTR_DB_OK, index_kind_insert(&tree, kind, rid3));

  struct ScanCtx {
    int count;
  };
  ScanCtx ctx = {0};

  auto cb = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };

  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&tree, &kind, cb, &ctx));
  EXPECT_EQ(3, ctx.count);
}

// ============================================================================
// Pubkey+Kind composite index tests
// ============================================================================
TEST_F(IndexTest, PubkeyKindInsertAndScan) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 36, sizeof(page_id_t),
                         BTREE_KEY_COMPOSITE));

  uint8_t pk[32];
  memset(pk, 0x33, 32);
  uint32_t kind = 1;
  RecordId rid1 = {10, 0};
  RecordId rid2 = {20, 1};

  ASSERT_EQ(NOSTR_DB_OK, index_pk_kind_insert(&tree, pk, kind, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_pk_kind_insert(&tree, pk, kind, rid2));

  // Build composite key and scan
  uint8_t composite[36];
  memcpy(composite, pk, 32);
  memcpy(composite + 32, &kind, sizeof(uint32_t));

  struct ScanCtx {
    int count;
  };
  ScanCtx ctx = {0};

  auto cb = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };

  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&tree, composite, cb, &ctx));
  EXPECT_EQ(2, ctx.count);
}

TEST_F(IndexTest, PubkeyKindDifferentKinds) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 36, sizeof(page_id_t),
                         BTREE_KEY_COMPOSITE));

  uint8_t pk[32];
  memset(pk, 0x44, 32);
  RecordId rid1 = {10, 0};
  RecordId rid2 = {20, 0};

  ASSERT_EQ(NOSTR_DB_OK, index_pk_kind_insert(&tree, pk, 1, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_pk_kind_insert(&tree, pk, 2, rid2));

  // Each kind has exactly 1 entry
  uint8_t key1[36], key2[36];
  memcpy(key1, pk, 32);
  uint32_t k1 = 1;
  memcpy(key1 + 32, &k1, 4);
  memcpy(key2, pk, 32);
  uint32_t k2 = 2;
  memcpy(key2 + 32, &k2, 4);

  struct ScanCtx {
    int count;
  };
  ScanCtx ctx1 = {0}, ctx2 = {0};

  auto cb = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };

  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&tree, key1, cb, &ctx1));
  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&tree, key2, cb, &ctx2));
  EXPECT_EQ(1, ctx1.count);
  EXPECT_EQ(1, ctx2.count);
}

// ============================================================================
// Tag index tests
// ============================================================================
TEST_F(IndexTest, TagInsertAndScan) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 33, sizeof(page_id_t),
                         BTREE_KEY_COMPOSITE_TAG));

  uint8_t  tag_val[32];
  memset(tag_val, 0x55, 32);
  RecordId rid1 = {10, 0};
  RecordId rid2 = {20, 1};

  ASSERT_EQ(NOSTR_DB_OK, index_tag_insert(&tree, 'e', tag_val, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_tag_insert(&tree, 'e', tag_val, rid2));

  uint8_t composite[33];
  composite[0] = 'e';
  memcpy(composite + 1, tag_val, 32);

  struct ScanCtx {
    int count;
  };
  ScanCtx ctx = {0};

  auto cb = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };

  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&tree, composite, cb, &ctx));
  EXPECT_EQ(2, ctx.count);
}

TEST_F(IndexTest, TagDifferentNames) {
  BTree tree;
  ASSERT_EQ(NOSTR_DB_OK,
            btree_create(&tree, &pool, 33, sizeof(page_id_t),
                         BTREE_KEY_COMPOSITE_TAG));

  uint8_t  tag_val[32];
  memset(tag_val, 0x66, 32);
  RecordId rid1 = {10, 0};
  RecordId rid2 = {20, 0};

  ASSERT_EQ(NOSTR_DB_OK, index_tag_insert(&tree, 'e', tag_val, rid1));
  ASSERT_EQ(NOSTR_DB_OK, index_tag_insert(&tree, 'p', tag_val, rid2));

  uint8_t key_e[33], key_p[33];
  key_e[0] = 'e';
  memcpy(key_e + 1, tag_val, 32);
  key_p[0] = 'p';
  memcpy(key_p + 1, tag_val, 32);

  struct ScanCtx {
    int count;
  };
  ScanCtx ctx_e = {0}, ctx_p = {0};

  auto cb = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };

  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&tree, key_e, cb, &ctx_e));
  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&tree, key_p, cb, &ctx_p));
  EXPECT_EQ(1, ctx_e.count);
  EXPECT_EQ(1, ctx_p.count);
}

// ============================================================================
// IndexManager integration tests
// ============================================================================
TEST_F(IndexTest, ManagerCreateAndFlush) {
  ASSERT_EQ(NOSTR_DB_OK, index_manager_create(&im, &pool));
  ASSERT_EQ(NOSTR_DB_OK, index_manager_flush(&im));
}

TEST_F(IndexTest, ManagerInsertEvent) {
  ASSERT_EQ(NOSTR_DB_OK, index_manager_create(&im, &pool));

  EventRecord rec;
  make_event(&rec, 0xAA, 0xBB, 1000, 1);

  RecordId rid = {10, 0};
  ASSERT_EQ(NOSTR_DB_OK,
            index_manager_insert_event(&im, rid, &rec, nullptr, 0));

  // Verify ID index
  RecordId found;
  ASSERT_EQ(NOSTR_DB_OK, index_id_lookup(&im.id_index, rec.id, &found));
  EXPECT_EQ(found.page_id, 10u);
  EXPECT_EQ(found.slot_index, 0u);

  // Verify kind index
  struct ScanCtx {
    int count;
  };
  ScanCtx ctx = {0};
  auto    cb  = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };
  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&im.kind_index, &rec.kind, cb, &ctx));
  EXPECT_EQ(1, ctx.count);
}

TEST_F(IndexTest, ManagerInsertEventWithTags) {
  ASSERT_EQ(NOSTR_DB_OK, index_manager_create(&im, &pool));

  EventRecord rec;
  make_event(&rec, 0xCC, 0xDD, 2000, 1);

  // Create tag data: tag 'e' with 64-hex-char value
  const char* hex_val =
    "aabbccddaabbccddaabbccddaabbccddaabbccddaabbccddaabbccddaabbccdd";
  auto tags = make_tags('e', hex_val);

  RecordId rid = {15, 2};
  ASSERT_EQ(NOSTR_DB_OK, index_manager_insert_event(
                            &im, rid, &rec, tags.data(), (uint16_t)tags.size()));

  // Verify tag index has the entry
  uint8_t expected_raw[32];
  // hex "aabb..." -> raw bytes
  for (int i = 0; i < 32; i++) {
    expected_raw[i] = 0xAA + (i % 2 == 0 ? 0 : 0x11);
  }
  // Actually parse properly
  for (int i = 0; i < 32; i++) {
    char h = hex_val[i * 2];
    char l = hex_val[i * 2 + 1];
    auto hv = [](char c) -> uint8_t {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      return 0;
    };
    expected_raw[i] = (uint8_t)((hv(h) << 4) | hv(l));
  }

  uint8_t tag_key[33];
  tag_key[0] = 'e';
  memcpy(tag_key + 1, expected_raw, 32);

  struct ScanCtx {
    int count;
  };
  ScanCtx ctx = {0};
  auto    cb  = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };
  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&im.tag_index, tag_key, cb, &ctx));
  EXPECT_EQ(1, ctx.count);
}

TEST_F(IndexTest, ManagerDeleteEvent) {
  ASSERT_EQ(NOSTR_DB_OK, index_manager_create(&im, &pool));

  EventRecord rec;
  make_event(&rec, 0xEE, 0xFF, 3000, 2);

  RecordId rid = {25, 1};
  ASSERT_EQ(NOSTR_DB_OK,
            index_manager_insert_event(&im, rid, &rec, nullptr, 0));

  // Delete
  ASSERT_EQ(NOSTR_DB_OK,
            index_manager_delete_event(&im, rid, &rec, nullptr, 0));

  // ID should be gone
  RecordId found;
  EXPECT_EQ(NOSTR_DB_ERROR_NOT_FOUND,
            index_id_lookup(&im.id_index, rec.id, &found));
}

TEST_F(IndexTest, ManagerMultipleEvents) {
  ASSERT_EQ(NOSTR_DB_OK, index_manager_create(&im, &pool));

  // Insert 10 events
  for (int i = 0; i < 10; i++) {
    EventRecord rec;
    make_event(&rec, (uint8_t)i, (uint8_t)(i % 3), (int64_t)(1000 + i * 100),
               (uint32_t)(i % 2));

    RecordId rid = {(page_id_t)(100 + i), (uint16_t)i};
    ASSERT_EQ(NOSTR_DB_OK,
              index_manager_insert_event(&im, rid, &rec, nullptr, 0));
  }

  // Verify: each ID is unique and findable
  for (int i = 0; i < 10; i++) {
    uint8_t id[32];
    memset(id, (uint8_t)i, 32);
    RecordId found;
    ASSERT_EQ(NOSTR_DB_OK, index_id_lookup(&im.id_index, id, &found));
    EXPECT_EQ(found.page_id, (page_id_t)(100 + i));
  }

  // Verify: kind 0 has 5 events, kind 1 has 5 events
  struct ScanCtx {
    int count;
  };
  auto cb = [](const void*, const void*, void* ud) -> bool {
    static_cast<ScanCtx*>(ud)->count++;
    return true;
  };

  uint32_t kind0 = 0;
  ScanCtx  ctx0  = {0};
  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&im.kind_index, &kind0, cb, &ctx0));
  EXPECT_EQ(5, ctx0.count);

  uint32_t kind1 = 1;
  ScanCtx  ctx1  = {0};
  ASSERT_EQ(NOSTR_DB_OK, btree_scan_key(&im.kind_index, &kind1, cb, &ctx1));
  EXPECT_EQ(5, ctx1.count);
}

TEST_F(IndexTest, ManagerOpenExisting) {
  ASSERT_EQ(NOSTR_DB_OK, index_manager_create(&im, &pool));

  EventRecord rec;
  make_event(&rec, 0x11, 0x22, 5000, 7);
  RecordId rid = {50, 3};
  ASSERT_EQ(NOSTR_DB_OK,
            index_manager_insert_event(&im, rid, &rec, nullptr, 0));

  // Flush and get meta pages
  ASSERT_EQ(NOSTR_DB_OK, index_manager_flush(&im));

  page_id_t id_meta      = im.id_index.meta_page;
  page_id_t tl_meta      = im.timeline_index.meta_page;
  page_id_t pk_meta      = im.pubkey_index.meta_page;
  page_id_t kind_meta    = im.kind_index.meta_page;
  page_id_t pkk_meta     = im.pubkey_kind_index.meta_page;
  page_id_t tag_meta     = im.tag_index.meta_page;

  // Flush buffer pool to disk
  ASSERT_EQ(NOSTR_DB_OK, buffer_pool_flush_all(&pool));

  // Re-open
  IndexManager im2;
  ASSERT_EQ(NOSTR_DB_OK,
            index_manager_open(&im2, &pool, id_meta, tl_meta, pk_meta,
                               kind_meta, pkk_meta, tag_meta));

  // Verify the event is still findable
  RecordId found;
  ASSERT_EQ(NOSTR_DB_OK, index_id_lookup(&im2.id_index, rec.id, &found));
  EXPECT_EQ(found.page_id, 50u);
  EXPECT_EQ(found.slot_index, 3u);
}
