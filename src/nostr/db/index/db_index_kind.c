#include "db_index_kind.h"

#include "../../../arch/memory.h"
#include "../../../util/string.h"
#include "../db_internal.h"

// Kind table size: 65536 slots * 16 bytes = 1MB
#define KIND_TABLE_SIZE (65536 * sizeof(NostrDBKindSlot))

// ============================================================================
// Helper: Get slots pointer (after header)
// ============================================================================
static NostrDBKindSlot* get_slots(NostrDB* db)
{
  return (NostrDBKindSlot*)((uint8_t*)db->kind_idx_map + sizeof(NostrDBIndexHeader));
}

// ============================================================================
// Helper: Get entry pool pointer (after slots)
// ============================================================================
static NostrDBKindEntry* get_entry_pool(NostrDB* db)
{
  return (NostrDBKindEntry*)((uint8_t*)db->kind_idx_map + sizeof(NostrDBIndexHeader) + KIND_TABLE_SIZE);
}

// ============================================================================
// Helper: Get entry at offset
// ============================================================================
static NostrDBKindEntry* get_entry_at(NostrDB* db, uint64_t offset)
{
  if (offset == 0) {
    return NULL;
  }
  return (NostrDBKindEntry*)((uint8_t*)db->kind_idx_map + offset);
}

// ============================================================================
// nostr_db_kind_index_init
// ============================================================================
int32_t nostr_db_kind_index_init(NostrDB* db)
{
  require_not_null(db, -1);

  NostrDBIndexHeader* header = db->kind_idx_header;
  require_not_null(header, -1);

  // Check if we have enough space for kind table + some entries
  size_t required_min = sizeof(NostrDBIndexHeader) + KIND_TABLE_SIZE;
  if (db->kind_idx_map_size < required_min) {
    return -1;
  }

  // Initialize header
  header->bucket_count     = 65536;  // Number of kinds
  header->entry_count      = 0;
  header->pool_next_offset = sizeof(NostrDBIndexHeader) + KIND_TABLE_SIZE;
  header->pool_size        = db->kind_idx_map_size - required_min;

  // Clear all slots
  NostrDBKindSlot* slots = get_slots(db);
  internal_memset(slots, 0, KIND_TABLE_SIZE);

  return 0;
}

// ============================================================================
// nostr_db_kind_index_get_slot
// ============================================================================
NostrDBKindSlot* nostr_db_kind_index_get_slot(NostrDB* db, uint32_t kind)
{
  require_not_null(db, NULL);

  if (kind > NOSTR_DB_KIND_MAX) {
    return NULL;
  }

  NostrDBKindSlot* slots = get_slots(db);
  return &slots[kind];
}

// ============================================================================
// nostr_db_kind_index_insert
// ============================================================================
int32_t nostr_db_kind_index_insert(
  NostrDB* db,
  uint32_t kind,
  uint64_t event_offset,
  int64_t  created_at)
{
  require_not_null(db, -1);

  if (kind > NOSTR_DB_KIND_MAX) {
    return -1;
  }

  NostrDBIndexHeader* header = db->kind_idx_header;

  // Initialize if not yet done
  if (header->bucket_count == 0) {
    if (nostr_db_kind_index_init(db) < 0) {
      return -1;
    }
  }

  // Check if we have space for a new entry
  size_t entry_size = sizeof(NostrDBKindEntry);
  if (header->pool_next_offset + entry_size > db->kind_idx_map_size) {
    return -1;  // Pool is full
  }

  // Allocate new entry
  uint64_t          new_entry_offset = header->pool_next_offset;
  NostrDBKindEntry* new_entry        = get_entry_at(db, new_entry_offset);
  header->pool_next_offset += entry_size;

  // Get the slot for this kind
  NostrDBKindSlot* slot = nostr_db_kind_index_get_slot(db, kind);

  // Initialize entry
  new_entry->event_offset       = event_offset;
  new_entry->created_at         = created_at;
  new_entry->older_entry_offset = slot->newest_entry_offset;  // Link to previous newest

  // Update slot
  slot->newest_entry_offset = new_entry_offset;
  slot->event_count++;

  header->entry_count++;

  return 0;
}

// ============================================================================
// nostr_db_kind_index_iterate
// ============================================================================
uint64_t nostr_db_kind_index_iterate(
  NostrDB*                    db,
  uint32_t                    kind,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data)
{
  require_not_null(db, 0);
  require_not_null(callback, 0);

  if (kind > NOSTR_DB_KIND_MAX) {
    return 0;
  }

  NostrDBKindSlot* slot = nostr_db_kind_index_get_slot(db, kind);
  if (slot->event_count == 0) {
    return 0;
  }

  uint64_t count        = 0;
  uint64_t entry_offset = slot->newest_entry_offset;

  while (entry_offset != 0) {
    if (limit > 0 && count >= limit) {
      break;
    }

    NostrDBKindEntry* entry = get_entry_at(db, entry_offset);
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
        break;  // Callback requested stop
      }
      count++;
    }

    entry_offset = entry->older_entry_offset;
  }

  return count;
}
