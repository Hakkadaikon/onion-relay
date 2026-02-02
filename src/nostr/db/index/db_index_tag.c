#include "db_index_tag.h"

#include "../../../arch/memory.h"
#include "../../../util/string.h"
#include "../db_internal.h"

// Entry structure for tag (same as pubkey entry)
typedef struct {
  uint64_t event_offset;
  int64_t  created_at;
  uint64_t older_entry_offset;
} NostrDBTagEntry;

// ============================================================================
// Helper: Compare tag name + value
// ============================================================================
static bool tag_equals(uint8_t a_name, const uint8_t* a_value,
                       uint8_t b_name, const uint8_t* b_value)
{
  if (a_name != b_name) {
    return false;
  }
  return internal_memcmp(a_value, b_value, 32) == 0;
}

// ============================================================================
// Helper: Get buckets pointer
// ============================================================================
static NostrDBTagBucket* get_buckets(NostrDB* db)
{
  return (NostrDBTagBucket*)((uint8_t*)db->tag_idx_map + sizeof(NostrDBIndexHeader));
}

// ============================================================================
// Helper: Get entry at offset
// ============================================================================
static NostrDBTagEntry* get_entry_at(NostrDB* db, uint64_t offset)
{
  if (offset == 0) {
    return NULL;
  }
  return (NostrDBTagEntry*)((uint8_t*)db->tag_idx_map + offset);
}

// ============================================================================
// nostr_db_tag_index_init
// ============================================================================
int32_t nostr_db_tag_index_init(NostrDB* db, uint64_t bucket_count)
{
  require_not_null(db, -1);
  require(bucket_count > 0, -1);

  NostrDBIndexHeader* header = db->tag_idx_header;
  require_not_null(header, -1);

  size_t buckets_size = bucket_count * sizeof(NostrDBTagBucket);
  size_t required_min = sizeof(NostrDBIndexHeader) + buckets_size;
  if (required_min > db->tag_idx_map_size) {
    return -1;
  }

  header->bucket_count     = bucket_count;
  header->entry_count      = 0;
  header->pool_next_offset = required_min;
  header->pool_size        = db->tag_idx_map_size - required_min;

  NostrDBTagBucket* buckets = get_buckets(db);
  internal_memset(buckets, 0, buckets_size);

  return 0;
}

// ============================================================================
// nostr_db_tag_index_hash
// ============================================================================
uint64_t nostr_db_tag_index_hash(uint8_t tag_name, const uint8_t* tag_value)
{
  uint64_t hash = (uint64_t)tag_name;
  for (int i = 0; i < 8; i++) {
    hash |= ((uint64_t)tag_value[i]) << ((i + 1) * 8);
  }
  return hash;
}

// ============================================================================
// Helper: Lookup tag in index
// ============================================================================
static NostrDBTagBucket* tag_index_lookup(NostrDB* db, uint8_t tag_name, const uint8_t* tag_value)
{
  require_not_null(db, NULL);
  require_not_null(tag_value, NULL);

  NostrDBIndexHeader* header = db->tag_idx_header;
  if (header->bucket_count == 0) {
    return NULL;
  }

  NostrDBTagBucket* buckets     = get_buckets(db);
  uint64_t          hash        = nostr_db_tag_index_hash(tag_name, tag_value);
  uint64_t          index       = hash % header->bucket_count;
  uint64_t          start_index = index;

  do {
    NostrDBTagBucket* bucket = &buckets[index];

    if (bucket->state == NOSTR_DB_BUCKET_STATE_EMPTY) {
      return NULL;
    }

    if (bucket->state == NOSTR_DB_BUCKET_STATE_USED) {
      if (tag_equals(bucket->tag_name, bucket->tag_value, tag_name, tag_value)) {
        return bucket;
      }
    }

    index = (index + 1) % header->bucket_count;
  } while (index != start_index);

  return NULL;
}

// ============================================================================
// nostr_db_tag_index_insert
// ============================================================================
int32_t nostr_db_tag_index_insert(
  NostrDB*       db,
  uint8_t        tag_name,
  const uint8_t* tag_value,
  uint64_t       event_offset,
  int64_t        created_at)
{
  require_not_null(db, -1);
  require_not_null(tag_value, -1);

  NostrDBIndexHeader* header = db->tag_idx_header;

  // Initialize if not yet done
  if (header->bucket_count == 0) {
    size_t   available    = db->tag_idx_map_size - sizeof(NostrDBIndexHeader);
    uint64_t bucket_count = (available / 2) / sizeof(NostrDBTagBucket);
    if (bucket_count == 0) {
      return -1;
    }
    if (nostr_db_tag_index_init(db, bucket_count) < 0) {
      return -1;
    }
  }

  // Check space for entry
  size_t entry_size = sizeof(NostrDBTagEntry);
  if (header->pool_next_offset + entry_size > db->tag_idx_map_size) {
    return -1;
  }

  // Lookup or create bucket
  NostrDBTagBucket* bucket = tag_index_lookup(db, tag_name, tag_value);

  if (is_null(bucket)) {
    NostrDBTagBucket* buckets     = get_buckets(db);
    uint64_t          hash        = nostr_db_tag_index_hash(tag_name, tag_value);
    uint64_t          index       = hash % header->bucket_count;
    uint64_t          start_index = index;

    do {
      bucket = &buckets[index];
      if (bucket->state == NOSTR_DB_BUCKET_STATE_EMPTY ||
          bucket->state == NOSTR_DB_BUCKET_STATE_TOMBSTONE) {
        break;
      }
      index = (index + 1) % header->bucket_count;
    } while (index != start_index);

    if (bucket->state != NOSTR_DB_BUCKET_STATE_EMPTY &&
        bucket->state != NOSTR_DB_BUCKET_STATE_TOMBSTONE) {
      return -1;
    }

    bucket->tag_name = tag_name;
    internal_memcpy(bucket->tag_value, tag_value, 32);
    bucket->newest_entry_offset = 0;
    bucket->event_count         = 0;
    bucket->state               = NOSTR_DB_BUCKET_STATE_USED;
  }

  // Allocate entry
  uint64_t         new_entry_offset = header->pool_next_offset;
  NostrDBTagEntry* new_entry        = get_entry_at(db, new_entry_offset);
  header->pool_next_offset += entry_size;

  new_entry->event_offset       = event_offset;
  new_entry->created_at         = created_at;
  new_entry->older_entry_offset = bucket->newest_entry_offset;

  bucket->newest_entry_offset = new_entry_offset;
  bucket->event_count++;

  header->entry_count++;

  return 0;
}

// ============================================================================
// nostr_db_tag_index_iterate
// ============================================================================
uint64_t nostr_db_tag_index_iterate(
  NostrDB*                    db,
  uint8_t                     tag_name,
  const uint8_t*              tag_value,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data)
{
  require_not_null(db, 0);
  require_not_null(tag_value, 0);
  require_not_null(callback, 0);

  NostrDBTagBucket* bucket = tag_index_lookup(db, tag_name, tag_value);
  if (is_null(bucket) || bucket->event_count == 0) {
    return 0;
  }

  uint64_t count        = 0;
  uint64_t entry_offset = bucket->newest_entry_offset;

  while (entry_offset != 0) {
    if (limit > 0 && count >= limit) {
      break;
    }

    NostrDBTagEntry* entry = get_entry_at(db, entry_offset);
    if (is_null(entry)) {
      break;
    }

    bool include = true;
    if (since > 0 && entry->created_at < since) {
      include = false;
    }
    if (until > 0 && entry->created_at > until) {
      include = false;
    }

    if (include) {
      if (!callback(entry->event_offset, entry->created_at, user_data)) {
        break;
      }
      count++;
    }

    entry_offset = entry->older_entry_offset;
  }

  return count;
}
