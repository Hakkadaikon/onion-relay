#ifndef NOSTR_DB_INDEX_KIND_H_
#define NOSTR_DB_INDEX_KIND_H_

#include "../../../util/types.h"
#include "../db_types.h"
#include "db_index_types.h"

// Forward declaration
struct NostrDB;
typedef struct NostrDB NostrDB;

// Maximum kind value (16-bit)
#define NOSTR_DB_KIND_MAX 65535

/**
 * @brief Initialize kind index
 * @param db Database instance
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_kind_index_init(NostrDB* db);

/**
 * @brief Get slot for a kind
 * @param db Database instance
 * @param kind Event kind
 * @return Pointer to slot, or NULL if invalid kind
 */
NostrDBKindSlot* nostr_db_kind_index_get_slot(NostrDB* db, uint32_t kind);

/**
 * @brief Insert entry into kind index
 * @param db Database instance
 * @param kind Event kind
 * @param event_offset Offset in events.dat
 * @param created_at Event timestamp
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_kind_index_insert(
  NostrDB* db,
  uint32_t kind,
  uint64_t event_offset,
  int64_t  created_at);

/**
 * @brief Iterate over events of a specific kind
 * @param db Database instance
 * @param kind Event kind
 * @param since Start timestamp (0 = no limit)
 * @param until End timestamp (0 = no limit)
 * @param limit Maximum entries (0 = no limit)
 * @param callback Function to call for each entry
 * @param user_data User data passed to callback
 * @return Number of entries iterated
 */
uint64_t nostr_db_kind_index_iterate(
  NostrDB*                    db,
  uint32_t                    kind,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data);

#endif
