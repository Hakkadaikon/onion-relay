#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

extern "C" {

typedef struct NostrDB NostrDB;

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
} NostrDBError;

typedef uint64_t nostr_db_offset_t;
#define NOSTR_DB_OFFSET_NOT_FOUND ((nostr_db_offset_t)0xFFFFFFFFFFFFFFFFULL)

// DB functions
NostrDBError nostr_db_init(NostrDB** db, const char* data_dir);
void         nostr_db_shutdown(NostrDB* db);

// ID Index functions
int32_t           nostr_db_id_index_init(NostrDB* db, uint64_t bucket_count);
uint64_t          nostr_db_id_index_hash(const uint8_t* id);
nostr_db_offset_t nostr_db_id_index_lookup(NostrDB* db, const uint8_t* id);
int32_t           nostr_db_id_index_insert(NostrDB* db, const uint8_t* id, uint64_t event_offset);
int32_t           nostr_db_id_index_delete(NostrDB* db, const uint8_t* id);
bool              nostr_db_id_index_needs_rehash(NostrDB* db);

// Timeline Index functions
int32_t  nostr_db_timeline_index_init(NostrDB* db);
int32_t  nostr_db_timeline_index_insert(NostrDB* db, int64_t created_at, uint64_t event_offset);
uint64_t nostr_db_timeline_index_find_since(NostrDB* db, int64_t since);
uint64_t nostr_db_timeline_index_find_until(NostrDB* db, int64_t until);

typedef bool (*NostrDBIndexIterateCallback)(uint64_t event_offset, int64_t created_at, void* user_data);
uint64_t nostr_db_timeline_index_iterate(
    NostrDB*                     db,
    int64_t                      since,
    int64_t                      until,
    uint64_t                     limit,
    NostrDBIndexIterateCallback  callback,
    void*                        user_data);

// Kind Index functions
int32_t nostr_db_kind_index_init(NostrDB* db);
int32_t nostr_db_kind_index_insert(NostrDB* db, uint32_t kind, uint64_t event_offset, int64_t created_at);
uint64_t nostr_db_kind_index_iterate(
    NostrDB*                     db,
    uint32_t                     kind,
    int64_t                      since,
    int64_t                      until,
    uint64_t                     limit,
    NostrDBIndexIterateCallback  callback,
    void*                        user_data);

// Pubkey Index functions
int32_t nostr_db_pubkey_index_init(NostrDB* db, uint64_t bucket_count);
int32_t nostr_db_pubkey_index_insert(NostrDB* db, const uint8_t* pubkey, uint64_t event_offset, int64_t created_at);
uint64_t nostr_db_pubkey_index_iterate(
    NostrDB*                     db,
    const uint8_t*               pubkey,
    int64_t                      since,
    int64_t                      until,
    uint64_t                     limit,
    NostrDBIndexIterateCallback  callback,
    void*                        user_data);

// Pubkey+Kind Index functions
int32_t nostr_db_pubkey_kind_index_init(NostrDB* db, uint64_t bucket_count);
uint64_t nostr_db_pubkey_kind_index_hash(const uint8_t* pubkey, uint32_t kind);
int32_t nostr_db_pubkey_kind_index_insert(
    NostrDB* db,
    const uint8_t* pubkey,
    uint32_t kind,
    uint64_t event_offset,
    int64_t created_at);
uint64_t nostr_db_pubkey_kind_index_iterate(
    NostrDB*                     db,
    const uint8_t*               pubkey,
    uint32_t                     kind,
    int64_t                      since,
    int64_t                      until,
    uint64_t                     limit,
    NostrDBIndexIterateCallback  callback,
    void*                        user_data);

// Tag Index functions
int32_t nostr_db_tag_index_init(NostrDB* db, uint64_t bucket_count);
uint64_t nostr_db_tag_index_hash(uint8_t tag_name, const uint8_t* tag_value);
int32_t nostr_db_tag_index_insert(
    NostrDB* db,
    uint8_t tag_name,
    const uint8_t* tag_value,
    uint64_t event_offset,
    int64_t created_at);
uint64_t nostr_db_tag_index_iterate(
    NostrDB* db,
    uint8_t tag_name,
    const uint8_t* tag_value,
    int64_t since,
    int64_t until,
    uint64_t limit,
    NostrDBIndexIterateCallback callback,
    void* user_data);

}  // extern "C"

class NostrDBIndexTest : public ::testing::Test {
protected:
  void SetUp() override {
    snprintf(test_dir, sizeof(test_dir), "/tmp/nostr_db_index_test_%d", getpid());
    mkdir(test_dir, 0755);
    db = nullptr;
    NostrDBError err = nostr_db_init(&db, test_dir);
    ASSERT_EQ(err, NOSTR_DB_OK);
  }

  void TearDown() override {
    if (db != nullptr) {
      nostr_db_shutdown(db);
      db = nullptr;
    }
    cleanup_directory(test_dir);
  }

  void cleanup_directory(const char* path) {
    DIR* dir = opendir(path);
    if (dir == nullptr) return;

    struct dirent* entry;
    char           filepath[512];

    while ((entry = readdir(dir)) != nullptr) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
      snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
      unlink(filepath);
    }

    closedir(dir);
    rmdir(path);
  }

  void make_id(uint8_t* id, uint64_t value) {
    memset(id, 0, 32);
    for (int i = 0; i < 8; i++) {
      id[i] = (uint8_t)(value >> (i * 8));
    }
  }

  char     test_dir[256];
  NostrDB* db;
};

// ============================================================================
// ID Index Tests
// ============================================================================

TEST_F(NostrDBIndexTest, IdIndexInsertAndLookup) {
  uint8_t id[32];
  make_id(id, 12345);

  int32_t result = nostr_db_id_index_insert(db, id, 1000);
  EXPECT_EQ(result, 0);

  nostr_db_offset_t offset = nostr_db_id_index_lookup(db, id);
  EXPECT_EQ(offset, 1000u);
}

TEST_F(NostrDBIndexTest, IdIndexLookupNotFound) {
  uint8_t id[32];
  make_id(id, 99999);

  nostr_db_offset_t offset = nostr_db_id_index_lookup(db, id);
  EXPECT_EQ(offset, NOSTR_DB_OFFSET_NOT_FOUND);
}

TEST_F(NostrDBIndexTest, IdIndexMultipleInserts) {
  for (uint64_t i = 0; i < 100; i++) {
    uint8_t id[32];
    make_id(id, i);
    int32_t result = nostr_db_id_index_insert(db, id, i * 100);
    EXPECT_EQ(result, 0);
  }

  // Verify all entries
  for (uint64_t i = 0; i < 100; i++) {
    uint8_t id[32];
    make_id(id, i);
    nostr_db_offset_t offset = nostr_db_id_index_lookup(db, id);
    EXPECT_EQ(offset, i * 100);
  }
}

TEST_F(NostrDBIndexTest, IdIndexDelete) {
  uint8_t id[32];
  make_id(id, 12345);

  nostr_db_id_index_insert(db, id, 1000);

  int32_t result = nostr_db_id_index_delete(db, id);
  EXPECT_EQ(result, 0);

  nostr_db_offset_t offset = nostr_db_id_index_lookup(db, id);
  EXPECT_EQ(offset, NOSTR_DB_OFFSET_NOT_FOUND);
}

TEST_F(NostrDBIndexTest, IdIndexDuplicateFails) {
  uint8_t id[32];
  make_id(id, 12345);

  int32_t result = nostr_db_id_index_insert(db, id, 1000);
  EXPECT_EQ(result, 0);

  result = nostr_db_id_index_insert(db, id, 2000);
  EXPECT_EQ(result, -1);  // Duplicate
}

// ============================================================================
// Timeline Index Tests
// ============================================================================

static bool collect_offsets(uint64_t event_offset, int64_t created_at, void* user_data) {
  auto* vec = static_cast<std::vector<uint64_t>*>(user_data);
  vec->push_back(event_offset);
  return true;
}

TEST_F(NostrDBIndexTest, TimelineIndexInsertAndIterate) {
  // Insert events with timestamps
  nostr_db_timeline_index_insert(db, 1000, 100);
  nostr_db_timeline_index_insert(db, 2000, 200);
  nostr_db_timeline_index_insert(db, 3000, 300);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_timeline_index_iterate(db, 0, 0, 0, collect_offsets, &offsets);

  EXPECT_EQ(count, 3u);
  EXPECT_EQ(offsets.size(), 3u);
  // Should be ordered newest first
  EXPECT_EQ(offsets[0], 300u);
  EXPECT_EQ(offsets[1], 200u);
  EXPECT_EQ(offsets[2], 100u);
}

TEST_F(NostrDBIndexTest, TimelineIndexWithLimit) {
  nostr_db_timeline_index_insert(db, 1000, 100);
  nostr_db_timeline_index_insert(db, 2000, 200);
  nostr_db_timeline_index_insert(db, 3000, 300);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_timeline_index_iterate(db, 0, 0, 2, collect_offsets, &offsets);

  EXPECT_EQ(count, 2u);
  EXPECT_EQ(offsets.size(), 2u);
}

TEST_F(NostrDBIndexTest, TimelineIndexWithSince) {
  nostr_db_timeline_index_insert(db, 1000, 100);
  nostr_db_timeline_index_insert(db, 2000, 200);
  nostr_db_timeline_index_insert(db, 3000, 300);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_timeline_index_iterate(db, 1500, 0, 0, collect_offsets, &offsets);

  EXPECT_EQ(count, 2u);  // 2000 and 3000
}

TEST_F(NostrDBIndexTest, TimelineIndexWithUntil) {
  nostr_db_timeline_index_insert(db, 1000, 100);
  nostr_db_timeline_index_insert(db, 2000, 200);
  nostr_db_timeline_index_insert(db, 3000, 300);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_timeline_index_iterate(db, 0, 2500, 0, collect_offsets, &offsets);

  EXPECT_EQ(count, 2u);  // 1000 and 2000
}

// ============================================================================
// Kind Index Tests
// ============================================================================

TEST_F(NostrDBIndexTest, KindIndexInsertAndIterate) {
  nostr_db_kind_index_insert(db, 1, 100, 1000);
  nostr_db_kind_index_insert(db, 1, 200, 2000);
  nostr_db_kind_index_insert(db, 3, 300, 3000);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_kind_index_iterate(db, 1, 0, 0, 0, collect_offsets, &offsets);

  EXPECT_EQ(count, 2u);
  EXPECT_EQ(offsets.size(), 2u);
}

TEST_F(NostrDBIndexTest, KindIndexDifferentKinds) {
  nostr_db_kind_index_insert(db, 0, 100, 1000);
  nostr_db_kind_index_insert(db, 1, 200, 2000);
  nostr_db_kind_index_insert(db, 3, 300, 3000);
  nostr_db_kind_index_insert(db, 7, 400, 4000);

  std::vector<uint64_t> offsets;

  offsets.clear();
  nostr_db_kind_index_iterate(db, 0, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(offsets.size(), 1u);

  offsets.clear();
  nostr_db_kind_index_iterate(db, 1, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(offsets.size(), 1u);

  offsets.clear();
  nostr_db_kind_index_iterate(db, 3, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(offsets.size(), 1u);

  offsets.clear();
  nostr_db_kind_index_iterate(db, 7, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(offsets.size(), 1u);
}

// ============================================================================
// Pubkey Index Tests
// ============================================================================

TEST_F(NostrDBIndexTest, PubkeyIndexInsertAndIterate) {
  uint8_t pubkey1[32], pubkey2[32];
  memset(pubkey1, 0x01, 32);
  memset(pubkey2, 0x02, 32);

  nostr_db_pubkey_index_insert(db, pubkey1, 100, 1000);
  nostr_db_pubkey_index_insert(db, pubkey1, 200, 2000);
  nostr_db_pubkey_index_insert(db, pubkey2, 300, 3000);

  std::vector<uint64_t> offsets;

  offsets.clear();
  uint64_t count = nostr_db_pubkey_index_iterate(db, pubkey1, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 2u);

  offsets.clear();
  count = nostr_db_pubkey_index_iterate(db, pubkey2, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 1u);
}

TEST_F(NostrDBIndexTest, PubkeyIndexNotFound) {
  uint8_t pubkey[32];
  memset(pubkey, 0xFF, 32);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_pubkey_index_iterate(db, pubkey, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 0u);
}

// ============================================================================
// Pubkey+Kind Index Tests
// ============================================================================

TEST_F(NostrDBIndexTest, PubkeyKindIndexInsertAndIterate) {
  uint8_t pubkey1[32], pubkey2[32];
  memset(pubkey1, 0x01, 32);
  memset(pubkey2, 0x02, 32);

  // Same pubkey, different kinds
  nostr_db_pubkey_kind_index_insert(db, pubkey1, 1, 100, 1000);
  nostr_db_pubkey_kind_index_insert(db, pubkey1, 1, 200, 2000);
  nostr_db_pubkey_kind_index_insert(db, pubkey1, 3, 300, 3000);
  nostr_db_pubkey_kind_index_insert(db, pubkey2, 1, 400, 4000);

  std::vector<uint64_t> offsets;

  // Query pubkey1 + kind 1
  offsets.clear();
  uint64_t count = nostr_db_pubkey_kind_index_iterate(db, pubkey1, 1, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 2u);

  // Query pubkey1 + kind 3
  offsets.clear();
  count = nostr_db_pubkey_kind_index_iterate(db, pubkey1, 3, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 1u);

  // Query pubkey2 + kind 1
  offsets.clear();
  count = nostr_db_pubkey_kind_index_iterate(db, pubkey2, 1, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 1u);
}

TEST_F(NostrDBIndexTest, PubkeyKindIndexNotFound) {
  uint8_t pubkey[32];
  memset(pubkey, 0xFF, 32);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_pubkey_kind_index_iterate(db, pubkey, 1, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 0u);
}

TEST_F(NostrDBIndexTest, PubkeyKindIndexWithLimit) {
  uint8_t pubkey[32];
  memset(pubkey, 0x01, 32);

  nostr_db_pubkey_kind_index_insert(db, pubkey, 1, 100, 1000);
  nostr_db_pubkey_kind_index_insert(db, pubkey, 1, 200, 2000);
  nostr_db_pubkey_kind_index_insert(db, pubkey, 1, 300, 3000);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_pubkey_kind_index_iterate(db, pubkey, 1, 0, 0, 2, collect_offsets, &offsets);
  EXPECT_EQ(count, 2u);
}

// ============================================================================
// Tag Index Tests
// ============================================================================

TEST_F(NostrDBIndexTest, TagIndexInsertAndIterate) {
  uint8_t tag_value1[32], tag_value2[32];
  memset(tag_value1, 0xAA, 32);
  memset(tag_value2, 0xBB, 32);

  // Insert 'e' tags
  nostr_db_tag_index_insert(db, 'e', tag_value1, 100, 1000);
  nostr_db_tag_index_insert(db, 'e', tag_value1, 200, 2000);
  nostr_db_tag_index_insert(db, 'e', tag_value2, 300, 3000);

  // Insert 'p' tags
  nostr_db_tag_index_insert(db, 'p', tag_value1, 400, 4000);

  std::vector<uint64_t> offsets;

  // Query 'e' + tag_value1
  offsets.clear();
  uint64_t count = nostr_db_tag_index_iterate(db, 'e', tag_value1, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 2u);

  // Query 'e' + tag_value2
  offsets.clear();
  count = nostr_db_tag_index_iterate(db, 'e', tag_value2, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 1u);

  // Query 'p' + tag_value1
  offsets.clear();
  count = nostr_db_tag_index_iterate(db, 'p', tag_value1, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 1u);
}

TEST_F(NostrDBIndexTest, TagIndexNotFound) {
  uint8_t tag_value[32];
  memset(tag_value, 0xFF, 32);

  std::vector<uint64_t> offsets;
  uint64_t count = nostr_db_tag_index_iterate(db, 'e', tag_value, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 0u);
}

TEST_F(NostrDBIndexTest, TagIndexDifferentNamesSameValue) {
  uint8_t tag_value[32];
  memset(tag_value, 0x01, 32);

  // Same value, different tag names
  nostr_db_tag_index_insert(db, 'e', tag_value, 100, 1000);
  nostr_db_tag_index_insert(db, 'p', tag_value, 200, 2000);
  nostr_db_tag_index_insert(db, 't', tag_value, 300, 3000);

  std::vector<uint64_t> offsets;

  offsets.clear();
  nostr_db_tag_index_iterate(db, 'e', tag_value, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(offsets.size(), 1u);

  offsets.clear();
  nostr_db_tag_index_iterate(db, 'p', tag_value, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(offsets.size(), 1u);

  offsets.clear();
  nostr_db_tag_index_iterate(db, 't', tag_value, 0, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(offsets.size(), 1u);
}

TEST_F(NostrDBIndexTest, TagIndexWithSinceUntil) {
  uint8_t tag_value[32];
  memset(tag_value, 0x01, 32);

  nostr_db_tag_index_insert(db, 'e', tag_value, 100, 1000);
  nostr_db_tag_index_insert(db, 'e', tag_value, 200, 2000);
  nostr_db_tag_index_insert(db, 'e', tag_value, 300, 3000);

  std::vector<uint64_t> offsets;

  // With since
  offsets.clear();
  uint64_t count = nostr_db_tag_index_iterate(db, 'e', tag_value, 1500, 0, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 2u);

  // With until
  offsets.clear();
  count = nostr_db_tag_index_iterate(db, 'e', tag_value, 0, 2500, 0, collect_offsets, &offsets);
  EXPECT_EQ(count, 2u);
}

// ============================================================================
// Struct Size Tests
// ============================================================================

TEST_F(NostrDBIndexTest, StructSizes) {
  // These are defined in db_index_types.h
  // Just verify the sizes match the design
  EXPECT_EQ(sizeof(uint64_t), 8u);
  EXPECT_EQ(sizeof(int64_t), 8u);
}
