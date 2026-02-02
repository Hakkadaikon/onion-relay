#ifndef NOSTR_DB_INDEX_ID_H_
#define NOSTR_DB_INDEX_ID_H_

#include "../../../util/types.h"
#include "../db_types.h"
#include "db_index_types.h"

// Forward declaration
struct NostrDB;
typedef struct NostrDB NostrDB;

/**
 * @brief Initialize ID index
 * @param db Database instance
 * @param bucket_count Number of hash buckets to initialize
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_id_index_init(NostrDB* db, uint64_t bucket_count);

/**
 * @brief Calculate hash for event ID
 * @param id 32-byte event ID
 * @return Hash value
 */
uint64_t nostr_db_id_index_hash(const uint8_t* id);

/**
 * @brief Lookup event by ID
 * @param db Database instance
 * @param id 32-byte event ID
 * @return Event offset, or NOSTR_DB_OFFSET_NOT_FOUND if not found
 */
nostr_db_offset_t nostr_db_id_index_lookup(NostrDB* db, const uint8_t* id);

/**
 * @brief Insert ID into index
 * @param db Database instance
 * @param id 32-byte event ID
 * @param event_offset Offset in events.dat
 * @return 0 on success, -1 on error (duplicate or full)
 */
int32_t nostr_db_id_index_insert(NostrDB* db, const uint8_t* id, uint64_t event_offset);

/**
 * @brief Delete ID from index (tombstone)
 * @param db Database instance
 * @param id 32-byte event ID
 * @return 0 on success, -1 on error (not found)
 */
int32_t nostr_db_id_index_delete(NostrDB* db, const uint8_t* id);

/**
 * @brief Check if index needs rehashing
 * @param db Database instance
 * @return true if rehash is needed
 */
bool nostr_db_id_index_needs_rehash(NostrDB* db);

#endif
