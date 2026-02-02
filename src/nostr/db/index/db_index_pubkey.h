#ifndef NOSTR_DB_INDEX_PUBKEY_H_
#define NOSTR_DB_INDEX_PUBKEY_H_

#include "../../../util/types.h"
#include "../db_types.h"
#include "db_index_types.h"

// Forward declaration
struct NostrDB;
typedef struct NostrDB NostrDB;

/**
 * @brief Initialize pubkey index
 * @param db Database instance
 * @param bucket_count Number of hash buckets
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_pubkey_index_init(NostrDB* db, uint64_t bucket_count);

/**
 * @brief Calculate hash for pubkey
 * @param pubkey 32-byte public key
 * @return Hash value
 */
uint64_t nostr_db_pubkey_index_hash(const uint8_t* pubkey);

/**
 * @brief Lookup pubkey in index
 * @param db Database instance
 * @param pubkey 32-byte public key
 * @return Pointer to bucket, or NULL if not found
 */
NostrDBPubkeyBucket* nostr_db_pubkey_index_lookup(NostrDB* db, const uint8_t* pubkey);

/**
 * @brief Insert entry into pubkey index
 * @param db Database instance
 * @param pubkey 32-byte public key
 * @param event_offset Offset in events.dat
 * @param created_at Event timestamp
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_pubkey_index_insert(
  NostrDB*       db,
  const uint8_t* pubkey,
  uint64_t       event_offset,
  int64_t        created_at);

/**
 * @brief Iterate over events from a specific pubkey
 * @param db Database instance
 * @param pubkey 32-byte public key
 * @param since Start timestamp (0 = no limit)
 * @param until End timestamp (0 = no limit)
 * @param limit Maximum entries (0 = no limit)
 * @param callback Function to call for each entry
 * @param user_data User data passed to callback
 * @return Number of entries iterated
 */
uint64_t nostr_db_pubkey_index_iterate(
  NostrDB*                    db,
  const uint8_t*              pubkey,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data);

#endif
