#include "db_index_timeline.h"

#include "../../../arch/memory.h"
#include "../../../util/string.h"
#include "../db_internal.h"

// ============================================================================
// Helper: Get entries pointer
// ============================================================================
static NostrDBTimelineEntry* get_entries(NostrDB* db)
{
  return (NostrDBTimelineEntry*)((uint8_t*)db->timeline_idx_map + sizeof(NostrDBIndexHeader));
}

// ============================================================================
// Helper: Get max entries count
// ============================================================================
static uint64_t get_max_entries(NostrDB* db)
{
  size_t available = db->timeline_idx_map_size - sizeof(NostrDBIndexHeader);
  return available / sizeof(NostrDBTimelineEntry);
}

// ============================================================================
// nostr_db_timeline_index_init
// ============================================================================
int32_t nostr_db_timeline_index_init(NostrDB* db)
{
  require_not_null(db, -1);

  NostrDBIndexHeader* header = db->timeline_idx_header;
  require_not_null(header, -1);

  // entry_count stores number of entries
  // pool_next_offset is not used for timeline (sorted array)
  header->entry_count = 0;

  return 0;
}

// ============================================================================
// nostr_db_timeline_index_insert
// ============================================================================
int32_t nostr_db_timeline_index_insert(NostrDB* db, int64_t created_at, uint64_t event_offset)
{
  require_not_null(db, -1);

  NostrDBIndexHeader*   header      = db->timeline_idx_header;
  NostrDBTimelineEntry* entries     = get_entries(db);
  uint64_t              entry_count = header->entry_count;
  uint64_t              max_entries = get_max_entries(db);

  if (entry_count >= max_entries) {
    return -1;  // Index is full
  }

  // Find insertion point using binary search (maintain sorted order by created_at DESC)
  // Newest entries are at the beginning
  uint64_t left  = 0;
  uint64_t right = entry_count;

  while (left < right) {
    uint64_t mid = left + (right - left) / 2;
    if (entries[mid].created_at > created_at) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  // Insert at position 'left'
  // Shift entries to make room
  if (left < entry_count) {
    // Move entries from left to end one position forward
    for (uint64_t i = entry_count; i > left; i--) {
      entries[i] = entries[i - 1];
    }
  }

  // Insert new entry
  entries[left].created_at   = created_at;
  entries[left].event_offset = event_offset;

  header->entry_count++;

  return 0;
}

// ============================================================================
// nostr_db_timeline_index_find_since
// ============================================================================
uint64_t nostr_db_timeline_index_find_since(NostrDB* db, int64_t since)
{
  require_not_null(db, 0);

  NostrDBIndexHeader*   header      = db->timeline_idx_header;
  NostrDBTimelineEntry* entries     = get_entries(db);
  uint64_t              entry_count = header->entry_count;

  if (entry_count == 0) {
    return 0;
  }

  // Binary search for last entry with created_at >= since
  // (entries are sorted DESC by created_at)
  uint64_t left  = 0;
  uint64_t right = entry_count;

  while (left < right) {
    uint64_t mid = left + (right - left) / 2;
    if (entries[mid].created_at >= since) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  // 'left' now points to first entry with created_at < since
  // So we return left (exclusive end index for iteration from 0)
  return left;
}

// ============================================================================
// nostr_db_timeline_index_find_until
// ============================================================================
uint64_t nostr_db_timeline_index_find_until(NostrDB* db, int64_t until)
{
  require_not_null(db, 0xFFFFFFFFFFFFFFFFULL);

  NostrDBIndexHeader*   header      = db->timeline_idx_header;
  NostrDBTimelineEntry* entries     = get_entries(db);
  uint64_t              entry_count = header->entry_count;

  if (entry_count == 0) {
    return 0xFFFFFFFFFFFFFFFFULL;
  }

  // Binary search for first entry with created_at <= until
  // (entries are sorted DESC by created_at)
  uint64_t left  = 0;
  uint64_t right = entry_count;

  while (left < right) {
    uint64_t mid = left + (right - left) / 2;
    if (entries[mid].created_at > until) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  // 'left' now points to first entry with created_at <= until
  if (left >= entry_count) {
    return 0xFFFFFFFFFFFFFFFFULL;  // No entry found
  }

  return left;
}

// ============================================================================
// nostr_db_timeline_index_iterate
// ============================================================================
uint64_t nostr_db_timeline_index_iterate(
  NostrDB*                    db,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data)
{
  require_not_null(db, 0);
  require_not_null(callback, 0);

  NostrDBIndexHeader*   header      = db->timeline_idx_header;
  NostrDBTimelineEntry* entries     = get_entries(db);
  uint64_t              entry_count = header->entry_count;

  if (entry_count == 0) {
    return 0;
  }

  // Find start index (first entry <= until, or 0 if no until)
  uint64_t start_idx = 0;
  if (until > 0) {
    start_idx = nostr_db_timeline_index_find_until(db, until);
    if (start_idx == 0xFFFFFFFFFFFFFFFFULL) {
      return 0;  // No entries <= until
    }
  }

  // Find end index (last entry >= since)
  uint64_t end_idx = entry_count;
  if (since > 0) {
    end_idx = nostr_db_timeline_index_find_since(db, since);
  }

  // Iterate from start_idx to end_idx (exclusive)
  uint64_t count = 0;
  for (uint64_t i = start_idx; i < end_idx; i++) {
    if (limit > 0 && count >= limit) {
      break;
    }

    NostrDBTimelineEntry* entry = &entries[i];
    if (!callback(entry->event_offset, entry->created_at, user_data)) {
      break;  // Callback requested stop
    }
    count++;
  }

  return count;
}
