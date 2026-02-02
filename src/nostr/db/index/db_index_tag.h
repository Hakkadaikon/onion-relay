#ifndef NOSTR_DB_INDEX_TAG_H_
#define NOSTR_DB_INDEX_TAG_H_

#include "../../../util/types.h"
#include "../db_types.h"
#include "db_index_types.h"

// Forward declaration
struct NostrDB;
typedef struct NostrDB NostrDB;

/**
 * @brief Initialize tag index
 * @param db Database instance
 * @param bucket_count Number of hash buckets
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_tag_index_init(NostrDB* db, uint64_t bucket_count);

/**
 * @brief Calculate hash for tag name + value
 * @param tag_name Tag name character (e.g., 'e', 'p')
 * @param tag_value Tag value (first 32 bytes)
 * @return Hash value
 */
uint64_t nostr_db_tag_index_hash(uint8_t tag_name, const uint8_t* tag_value);

/**
 * @brief Insert entry into tag index
 * @param db Database instance
 * @param tag_name Tag name character
 * @param tag_value Tag value (first 32 bytes used)
 * @param event_offset Offset in events.dat
 * @param created_at Event timestamp
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_tag_index_insert(
  NostrDB*       db,
  uint8_t        tag_name,
  const uint8_t* tag_value,
  uint64_t       event_offset,
  int64_t        created_at);

/**
 * @brief Iterate over events with a specific tag
 * @param db Database instance
 * @param tag_name Tag name character
 * @param tag_value Tag value (first 32 bytes used)
 * @param since Start timestamp (0 = no limit)
 * @param until End timestamp (0 = no limit)
 * @param limit Maximum entries (0 = no limit)
 * @param callback Function to call for each entry
 * @param user_data User data passed to callback
 * @return Number of entries iterated
 */
uint64_t nostr_db_tag_index_iterate(
  NostrDB*                    db,
  uint8_t                     tag_name,
  const uint8_t*              tag_value,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data);

#endif
