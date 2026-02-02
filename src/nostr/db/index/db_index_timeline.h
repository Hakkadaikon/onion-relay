#ifndef NOSTR_DB_INDEX_TIMELINE_H_
#define NOSTR_DB_INDEX_TIMELINE_H_

#include "../../../util/types.h"
#include "../db_types.h"
#include "db_index_types.h"

// Forward declaration
struct NostrDB;
typedef struct NostrDB NostrDB;

/**
 * @brief Initialize timeline index
 * @param db Database instance
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_timeline_index_init(NostrDB* db);

/**
 * @brief Insert entry into timeline index (maintains sorted order)
 * @param db Database instance
 * @param created_at Timestamp
 * @param event_offset Offset in events.dat
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_timeline_index_insert(NostrDB* db, int64_t created_at, uint64_t event_offset);

/**
 * @brief Find index of first entry >= since using binary search
 * @param db Database instance
 * @param since Timestamp to search for
 * @return Index of entry, or entry_count if not found
 */
uint64_t nostr_db_timeline_index_find_since(NostrDB* db, int64_t since);

/**
 * @brief Find index of last entry <= until using binary search
 * @param db Database instance
 * @param until Timestamp to search for
 * @return Index of entry, or UINT64_MAX if not found
 */
uint64_t nostr_db_timeline_index_find_until(NostrDB* db, int64_t until);

/**
 * @brief Iterate over timeline entries in time range
 * @param db Database instance
 * @param since Start timestamp (0 = no limit)
 * @param until End timestamp (0 = no limit)
 * @param limit Maximum entries to return (0 = no limit)
 * @param callback Function to call for each entry
 * @param user_data User data passed to callback
 * @return Number of entries iterated
 */
uint64_t nostr_db_timeline_index_iterate(
  NostrDB*                    db,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data);

#endif
