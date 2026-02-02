#include "db_index_pubkey.h"

#include "../../../arch/memory.h"
#include "../../../util/string.h"
#include "../db_internal.h"

// ============================================================================
// Helper: Compare two 32-byte keys
// ============================================================================
static bool key_equals(const uint8_t* a, const uint8_t* b)
{
  return internal_memcmp(a, b, 32) == 0;
}

// ============================================================================
// Helper: Get buckets pointer
// ============================================================================
static NostrDBPubkeyBucket* get_buckets(NostrDB* db)
{
  return (NostrDBPubkeyBucket*)((uint8_t*)db->pubkey_idx_map + sizeof(NostrDBIndexHeader));
}

// ============================================================================
// Helper: Get entry pool start
// ============================================================================
static uint64_t get_pool_start(NostrDB* db)
{
  NostrDBIndexHeader* header = db->pubkey_idx_header;
  return sizeof(NostrDBIndexHeader) + header->bucket_count * sizeof(NostrDBPubkeyBucket);
}

// ============================================================================
// Helper: Get entry at offset
// ============================================================================
static NostrDBPubkeyEntry* get_entry_at(NostrDB* db, uint64_t offset)
{
  if (offset == 0) {
    return NULL;
  }
  return (NostrDBPubkeyEntry*)((uint8_t*)db->pubkey_idx_map + offset);
}

// ============================================================================
// nostr_db_pubkey_index_init
// ============================================================================
int32_t nostr_db_pubkey_index_init(NostrDB* db, uint64_t bucket_count)
{
  require_not_null(db, -1);
  require(bucket_count > 0, -1);

  NostrDBIndexHeader* header = db->pubkey_idx_header;
  require_not_null(header, -1);

  // Calculate required size for buckets
  size_t buckets_size = bucket_count * sizeof(NostrDBPubkeyBucket);
  size_t required_min = sizeof(NostrDBIndexHeader) + buckets_size;
  if (required_min > db->pubkey_idx_map_size) {
    return -1;
  }

  // Initialize header
  header->bucket_count     = bucket_count;
  header->entry_count      = 0;
  header->pool_next_offset = required_min;
  header->pool_size        = db->pubkey_idx_map_size - required_min;

  // Clear all buckets
  NostrDBPubkeyBucket* buckets = get_buckets(db);
  internal_memset(buckets, 0, buckets_size);

  return 0;
}

// ============================================================================
// nostr_db_pubkey_index_hash
// ============================================================================
uint64_t nostr_db_pubkey_index_hash(const uint8_t* pubkey)
{
  // Use first 8 bytes as hash
  uint64_t hash = 0;
  for (int i = 0; i < 8; i++) {
    hash |= ((uint64_t)pubkey[i]) << (i * 8);
  }
  return hash;
}

// ============================================================================
// nostr_db_pubkey_index_lookup
// ============================================================================
NostrDBPubkeyBucket* nostr_db_pubkey_index_lookup(NostrDB* db, const uint8_t* pubkey)
{
  require_not_null(db, NULL);
  require_not_null(pubkey, NULL);

  NostrDBIndexHeader* header = db->pubkey_idx_header;
  if (header->bucket_count == 0) {
    return NULL;
  }

  NostrDBPubkeyBucket* buckets     = get_buckets(db);
  uint64_t             hash        = nostr_db_pubkey_index_hash(pubkey);
  uint64_t             index       = hash % header->bucket_count;
  uint64_t             start_index = index;

  // Linear probing
  do {
    NostrDBPubkeyBucket* bucket = &buckets[index];

    if (bucket->state == NOSTR_DB_BUCKET_STATE_EMPTY) {
      return NULL;  // Not found
    }

    if (bucket->state == NOSTR_DB_BUCKET_STATE_USED) {
      if (key_equals(bucket->pubkey, pubkey)) {
        return bucket;
      }
    }

    index = (index + 1) % header->bucket_count;
  } while (index != start_index);

  return NULL;
}

// ============================================================================
// nostr_db_pubkey_index_insert
// ============================================================================
int32_t nostr_db_pubkey_index_insert(
  NostrDB*       db,
  const uint8_t* pubkey,
  uint64_t       event_offset,
  int64_t        created_at)
{
  require_not_null(db, -1);
  require_not_null(pubkey, -1);

  NostrDBIndexHeader* header = db->pubkey_idx_header;

  // Initialize if not yet done
  if (header->bucket_count == 0) {
    // Calculate default bucket count
    size_t available = db->pubkey_idx_map_size - sizeof(NostrDBIndexHeader);
    // Reserve half for buckets, half for entries
    uint64_t bucket_count = (available / 2) / sizeof(NostrDBPubkeyBucket);
    if (bucket_count == 0) {
      return -1;
    }
    if (nostr_db_pubkey_index_init(db, bucket_count) < 0) {
      return -1;
    }
  }

  // Check if we have space for a new entry
  size_t entry_size = sizeof(NostrDBPubkeyEntry);
  if (header->pool_next_offset + entry_size > db->pubkey_idx_map_size) {
    return -1;  // Pool is full
  }

  // Lookup or create bucket
  NostrDBPubkeyBucket* bucket = nostr_db_pubkey_index_lookup(db, pubkey);

  if (is_null(bucket)) {
    // Need to create new bucket entry
    NostrDBPubkeyBucket* buckets     = get_buckets(db);
    uint64_t             hash        = nostr_db_pubkey_index_hash(pubkey);
    uint64_t             index       = hash % header->bucket_count;
    uint64_t             start_index = index;

    // Find empty or tombstone slot
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
      return -1;  // Hash table is full
    }

    // Initialize bucket
    internal_memcpy(bucket->pubkey, pubkey, 32);
    bucket->newest_entry_offset = 0;
    bucket->event_count         = 0;
    bucket->state               = NOSTR_DB_BUCKET_STATE_USED;
  }

  // Allocate new entry
  uint64_t            new_entry_offset = header->pool_next_offset;
  NostrDBPubkeyEntry* new_entry        = get_entry_at(db, new_entry_offset);
  header->pool_next_offset += entry_size;

  // Initialize entry
  new_entry->event_offset       = event_offset;
  new_entry->created_at         = created_at;
  new_entry->older_entry_offset = bucket->newest_entry_offset;

  // Update bucket
  bucket->newest_entry_offset = new_entry_offset;
  bucket->event_count++;

  header->entry_count++;

  return 0;
}

// ============================================================================
// nostr_db_pubkey_index_iterate
// ============================================================================
uint64_t nostr_db_pubkey_index_iterate(
  NostrDB*                    db,
  const uint8_t*              pubkey,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data)
{
  require_not_null(db, 0);
  require_not_null(pubkey, 0);
  require_not_null(callback, 0);

  NostrDBPubkeyBucket* bucket = nostr_db_pubkey_index_lookup(db, pubkey);
  if (is_null(bucket) || bucket->event_count == 0) {
    return 0;
  }

  uint64_t count        = 0;
  uint64_t entry_offset = bucket->newest_entry_offset;

  while (entry_offset != 0) {
    if (limit > 0 && count >= limit) {
      break;
    }

    NostrDBPubkeyEntry* entry = get_entry_at(db, entry_offset);
    if (is_null(entry)) {
      break;
    }

    // Apply time filters
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
