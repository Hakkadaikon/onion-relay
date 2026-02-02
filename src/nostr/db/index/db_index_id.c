#include "db_index_id.h"

#include "../../../arch/memory.h"
#include "../../../util/string.h"
#include "../db_internal.h"

// ============================================================================
// Helper: Compare two 32-byte IDs
// ============================================================================
static bool id_equals(const uint8_t* a, const uint8_t* b)
{
  return internal_memcmp(a, b, 32) == 0;
}

// ============================================================================
// Helper: Get buckets pointer
// ============================================================================
static NostrDBIdBucket* get_buckets(NostrDB* db)
{
  return (NostrDBIdBucket*)((uint8_t*)db->id_idx_map + sizeof(NostrDBIndexHeader));
}

// ============================================================================
// nostr_db_id_index_init
// ============================================================================
int32_t nostr_db_id_index_init(NostrDB* db, uint64_t bucket_count)
{
  require_not_null(db, -1);
  require(bucket_count > 0, -1);

  NostrDBIndexHeader* header = db->id_idx_header;
  require_not_null(header, -1);

  // Calculate required size
  size_t required_size = sizeof(NostrDBIndexHeader) + bucket_count * sizeof(NostrDBIdBucket);
  if (required_size > db->id_idx_map_size) {
    return -1;  // Not enough space
  }

  // Initialize header
  header->bucket_count = bucket_count;
  header->entry_count  = 0;

  // Clear all buckets
  NostrDBIdBucket* buckets = get_buckets(db);
  internal_memset(buckets, 0, bucket_count * sizeof(NostrDBIdBucket));

  return 0;
}

// ============================================================================
// nostr_db_id_index_hash
// ============================================================================
uint64_t nostr_db_id_index_hash(const uint8_t* id)
{
  // Use first 8 bytes as hash (IDs are already random hashes)
  uint64_t hash = 0;
  for (int i = 0; i < 8; i++) {
    hash |= ((uint64_t)id[i]) << (i * 8);
  }
  return hash;
}

// ============================================================================
// nostr_db_id_index_lookup
// ============================================================================
nostr_db_offset_t nostr_db_id_index_lookup(NostrDB* db, const uint8_t* id)
{
  require_not_null(db, NOSTR_DB_OFFSET_NOT_FOUND);
  require_not_null(id, NOSTR_DB_OFFSET_NOT_FOUND);

  NostrDBIndexHeader* header = db->id_idx_header;
  if (header->bucket_count == 0) {
    return NOSTR_DB_OFFSET_NOT_FOUND;
  }

  NostrDBIdBucket* buckets     = get_buckets(db);
  uint64_t         hash        = nostr_db_id_index_hash(id);
  uint64_t         index       = hash % header->bucket_count;
  uint64_t         start_index = index;

  // Linear probing
  do {
    NostrDBIdBucket* bucket = &buckets[index];

    if (bucket->state == NOSTR_DB_BUCKET_STATE_EMPTY) {
      // Empty slot - not found
      return NOSTR_DB_OFFSET_NOT_FOUND;
    }

    if (bucket->state == NOSTR_DB_BUCKET_STATE_USED) {
      if (id_equals(bucket->id, id)) {
        return bucket->event_offset;
      }
    }
    // TOMBSTONE - continue searching

    index = (index + 1) % header->bucket_count;
  } while (index != start_index);

  return NOSTR_DB_OFFSET_NOT_FOUND;
}

// ============================================================================
// nostr_db_id_index_insert
// ============================================================================
int32_t nostr_db_id_index_insert(NostrDB* db, const uint8_t* id, uint64_t event_offset)
{
  require_not_null(db, -1);
  require_not_null(id, -1);

  NostrDBIndexHeader* header = db->id_idx_header;

  // Initialize if not yet done
  if (header->bucket_count == 0) {
    // Calculate default bucket count based on file size
    size_t   available   = db->id_idx_map_size - sizeof(NostrDBIndexHeader);
    uint64_t max_buckets = available / sizeof(NostrDBIdBucket);
    // Use ~70% capacity for load factor
    uint64_t bucket_count = max_buckets;
    if (bucket_count == 0) {
      return -1;
    }
    if (nostr_db_id_index_init(db, bucket_count) < 0) {
      return -1;
    }
  }

  // Check for duplicate
  if (nostr_db_id_index_lookup(db, id) != NOSTR_DB_OFFSET_NOT_FOUND) {
    return -1;  // Duplicate
  }

  NostrDBIdBucket* buckets         = get_buckets(db);
  uint64_t         hash            = nostr_db_id_index_hash(id);
  uint64_t         index           = hash % header->bucket_count;
  uint64_t         start_index     = index;
  int64_t          tombstone_index = -1;

  // Find empty or tombstone slot
  do {
    NostrDBIdBucket* bucket = &buckets[index];

    if (bucket->state == NOSTR_DB_BUCKET_STATE_EMPTY) {
      // Use this empty slot
      break;
    }

    if (bucket->state == NOSTR_DB_BUCKET_STATE_TOMBSTONE && tombstone_index < 0) {
      // Remember first tombstone for potential reuse
      tombstone_index = (int64_t)index;
    }

    index = (index + 1) % header->bucket_count;
  } while (index != start_index);

  // Use tombstone if found, otherwise use the current index
  if (tombstone_index >= 0) {
    index = (uint64_t)tombstone_index;
  } else if (buckets[index].state != NOSTR_DB_BUCKET_STATE_EMPTY) {
    return -1;  // Hash table is full
  }

  // Insert
  NostrDBIdBucket* bucket = &buckets[index];
  internal_memcpy(bucket->id, id, 32);
  bucket->event_offset = event_offset;
  bucket->state        = NOSTR_DB_BUCKET_STATE_USED;

  header->entry_count++;

  return 0;
}

// ============================================================================
// nostr_db_id_index_delete
// ============================================================================
int32_t nostr_db_id_index_delete(NostrDB* db, const uint8_t* id)
{
  require_not_null(db, -1);
  require_not_null(id, -1);

  NostrDBIndexHeader* header = db->id_idx_header;
  if (header->bucket_count == 0) {
    return -1;  // Not initialized
  }

  NostrDBIdBucket* buckets     = get_buckets(db);
  uint64_t         hash        = nostr_db_id_index_hash(id);
  uint64_t         index       = hash % header->bucket_count;
  uint64_t         start_index = index;

  // Find the entry
  do {
    NostrDBIdBucket* bucket = &buckets[index];

    if (bucket->state == NOSTR_DB_BUCKET_STATE_EMPTY) {
      return -1;  // Not found
    }

    if (bucket->state == NOSTR_DB_BUCKET_STATE_USED) {
      if (id_equals(bucket->id, id)) {
        // Found - mark as tombstone
        bucket->state = NOSTR_DB_BUCKET_STATE_TOMBSTONE;
        header->entry_count--;
        return 0;
      }
    }

    index = (index + 1) % header->bucket_count;
  } while (index != start_index);

  return -1;  // Not found
}

// ============================================================================
// nostr_db_id_index_needs_rehash
// ============================================================================
bool nostr_db_id_index_needs_rehash(NostrDB* db)
{
  require_not_null(db, false);

  NostrDBIndexHeader* header = db->id_idx_header;
  if (header->bucket_count == 0) {
    return false;
  }

  // Check load factor (70%)
  uint64_t threshold = (header->bucket_count * NOSTR_DB_HASH_LOAD_FACTOR_PERCENT) / 100;
  return header->entry_count >= threshold;
}
