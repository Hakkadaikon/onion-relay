#include "db_index_pubkey_kind.h"

#include "../../../arch/memory.h"
#include "../../../util/string.h"
#include "../db_internal.h"

// Entry structure for pubkey+kind (same as pubkey entry)
typedef struct {
  uint64_t event_offset;
  int64_t  created_at;
  uint64_t older_entry_offset;
} NostrDBPubkeyKindEntry;

// ============================================================================
// Helper: Compare pubkey+kind
// ============================================================================
static bool key_kind_equals(const uint8_t* a_pubkey, uint32_t a_kind,
                            const uint8_t* b_pubkey, uint32_t b_kind)
{
  if (a_kind != b_kind) {
    return false;
  }
  return internal_memcmp(a_pubkey, b_pubkey, 32) == 0;
}

// ============================================================================
// Helper: Get buckets pointer
// ============================================================================
static NostrDBPubkeyKindBucket* get_buckets(NostrDB* db)
{
  return (NostrDBPubkeyKindBucket*)((uint8_t*)db->pubkey_kind_idx_map + sizeof(NostrDBIndexHeader));
}

// ============================================================================
// Helper: Get entry at offset
// ============================================================================
static NostrDBPubkeyKindEntry* get_entry_at(NostrDB* db, uint64_t offset)
{
  if (offset == 0) {
    return NULL;
  }
  return (NostrDBPubkeyKindEntry*)((uint8_t*)db->pubkey_kind_idx_map + offset);
}

// ============================================================================
// nostr_db_pubkey_kind_index_init
// ============================================================================
int32_t nostr_db_pubkey_kind_index_init(NostrDB* db, uint64_t bucket_count)
{
  require_not_null(db, -1);
  require(bucket_count > 0, -1);

  NostrDBIndexHeader* header = db->pubkey_kind_idx_header;
  require_not_null(header, -1);

  size_t buckets_size = bucket_count * sizeof(NostrDBPubkeyKindBucket);
  size_t required_min = sizeof(NostrDBIndexHeader) + buckets_size;
  if (required_min > db->pubkey_kind_idx_map_size) {
    return -1;
  }

  header->bucket_count     = bucket_count;
  header->entry_count      = 0;
  header->pool_next_offset = required_min;
  header->pool_size        = db->pubkey_kind_idx_map_size - required_min;

  NostrDBPubkeyKindBucket* buckets = get_buckets(db);
  internal_memset(buckets, 0, buckets_size);

  return 0;
}

// ============================================================================
// nostr_db_pubkey_kind_index_hash
// ============================================================================
uint64_t nostr_db_pubkey_kind_index_hash(const uint8_t* pubkey, uint32_t kind)
{
  // Combine pubkey hash with kind
  uint64_t hash = 0;
  for (int i = 0; i < 8; i++) {
    hash |= ((uint64_t)pubkey[i]) << (i * 8);
  }
  // Mix in kind
  hash ^= ((uint64_t)kind * 0x9E3779B97F4A7C15ULL);  // Golden ratio hash
  return hash;
}

// ============================================================================
// nostr_db_pubkey_kind_index_lookup
// ============================================================================
NostrDBPubkeyKindBucket* nostr_db_pubkey_kind_index_lookup(
  NostrDB*       db,
  const uint8_t* pubkey,
  uint32_t       kind)
{
  require_not_null(db, NULL);
  require_not_null(pubkey, NULL);

  NostrDBIndexHeader* header = db->pubkey_kind_idx_header;
  if (header->bucket_count == 0) {
    return NULL;
  }

  NostrDBPubkeyKindBucket* buckets     = get_buckets(db);
  uint64_t                 hash        = nostr_db_pubkey_kind_index_hash(pubkey, kind);
  uint64_t                 index       = hash % header->bucket_count;
  uint64_t                 start_index = index;

  do {
    NostrDBPubkeyKindBucket* bucket = &buckets[index];

    if (bucket->state == NOSTR_DB_BUCKET_STATE_EMPTY) {
      return NULL;
    }

    if (bucket->state == NOSTR_DB_BUCKET_STATE_USED) {
      if (key_kind_equals(bucket->pubkey, bucket->kind, pubkey, kind)) {
        return bucket;
      }
    }

    index = (index + 1) % header->bucket_count;
  } while (index != start_index);

  return NULL;
}

// ============================================================================
// nostr_db_pubkey_kind_index_insert
// ============================================================================
int32_t nostr_db_pubkey_kind_index_insert(
  NostrDB*       db,
  const uint8_t* pubkey,
  uint32_t       kind,
  uint64_t       event_offset,
  int64_t        created_at)
{
  require_not_null(db, -1);
  require_not_null(pubkey, -1);

  NostrDBIndexHeader* header = db->pubkey_kind_idx_header;

  // Initialize if not yet done
  if (header->bucket_count == 0) {
    size_t   available    = db->pubkey_kind_idx_map_size - sizeof(NostrDBIndexHeader);
    uint64_t bucket_count = (available / 2) / sizeof(NostrDBPubkeyKindBucket);
    if (bucket_count == 0) {
      return -1;
    }
    if (nostr_db_pubkey_kind_index_init(db, bucket_count) < 0) {
      return -1;
    }
  }

  // Check space for entry
  size_t entry_size = sizeof(NostrDBPubkeyKindEntry);
  if (header->pool_next_offset + entry_size > db->pubkey_kind_idx_map_size) {
    return -1;
  }

  // Lookup or create bucket
  NostrDBPubkeyKindBucket* bucket = nostr_db_pubkey_kind_index_lookup(db, pubkey, kind);

  if (is_null(bucket)) {
    NostrDBPubkeyKindBucket* buckets     = get_buckets(db);
    uint64_t                 hash        = nostr_db_pubkey_kind_index_hash(pubkey, kind);
    uint64_t                 index       = hash % header->bucket_count;
    uint64_t                 start_index = index;

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

    internal_memcpy(bucket->pubkey, pubkey, 32);
    bucket->kind                = kind;
    bucket->newest_entry_offset = 0;
    bucket->event_count         = 0;
    bucket->state               = NOSTR_DB_BUCKET_STATE_USED;
  }

  // Allocate entry
  uint64_t                new_entry_offset = header->pool_next_offset;
  NostrDBPubkeyKindEntry* new_entry        = get_entry_at(db, new_entry_offset);
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
// nostr_db_pubkey_kind_index_iterate
// ============================================================================
uint64_t nostr_db_pubkey_kind_index_iterate(
  NostrDB*                    db,
  const uint8_t*              pubkey,
  uint32_t                    kind,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data)
{
  require_not_null(db, 0);
  require_not_null(pubkey, 0);
  require_not_null(callback, 0);

  NostrDBPubkeyKindBucket* bucket = nostr_db_pubkey_kind_index_lookup(db, pubkey, kind);
  if (is_null(bucket) || bucket->event_count == 0) {
    return 0;
  }

  uint64_t count        = 0;
  uint64_t entry_offset = bucket->newest_entry_offset;

  while (entry_offset != 0) {
    if (limit > 0 && count >= limit) {
      break;
    }

    NostrDBPubkeyKindEntry* entry = get_entry_at(db, entry_offset);
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
