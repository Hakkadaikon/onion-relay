#ifndef NOSTR_DB_INDEX_PUBKEY_KIND_H_
#define NOSTR_DB_INDEX_PUBKEY_KIND_H_

#include "../../../util/types.h"
#include "../db_types.h"
#include "db_index_types.h"

// Forward declaration
struct NostrDB;
typedef struct NostrDB NostrDB;

/**
 * @brief Initialize pubkey+kind index
 * @param db Database instance
 * @param bucket_count Number of hash buckets
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_pubkey_kind_index_init(NostrDB* db, uint64_t bucket_count);

/**
 * @brief Calculate hash for pubkey+kind combination
 * @param pubkey 32-byte public key
 * @param kind Event kind
 * @return Hash value
 */
uint64_t nostr_db_pubkey_kind_index_hash(const uint8_t* pubkey, uint32_t kind);

/**
 * @brief Lookup pubkey+kind in index
 * @param db Database instance
 * @param pubkey 32-byte public key
 * @param kind Event kind
 * @return Pointer to bucket, or NULL if not found
 */
NostrDBPubkeyKindBucket* nostr_db_pubkey_kind_index_lookup(
  NostrDB*       db,
  const uint8_t* pubkey,
  uint32_t       kind);

/**
 * @brief Insert entry into pubkey+kind index
 * @param db Database instance
 * @param pubkey 32-byte public key
 * @param kind Event kind
 * @param event_offset Offset in events.dat
 * @param created_at Event timestamp
 * @return 0 on success, -1 on error
 */
int32_t nostr_db_pubkey_kind_index_insert(
  NostrDB*       db,
  const uint8_t* pubkey,
  uint32_t       kind,
  uint64_t       event_offset,
  int64_t        created_at);

/**
 * @brief Iterate over events from a specific pubkey+kind
 * @param db Database instance
 * @param pubkey 32-byte public key
 * @param kind Event kind
 * @param since Start timestamp (0 = no limit)
 * @param until End timestamp (0 = no limit)
 * @param limit Maximum entries (0 = no limit)
 * @param callback Function to call for each entry
 * @param user_data User data passed to callback
 * @return Number of entries iterated
 */
uint64_t nostr_db_pubkey_kind_index_iterate(
  NostrDB*                    db,
  const uint8_t*              pubkey,
  uint32_t                    kind,
  int64_t                     since,
  int64_t                     until,
  uint64_t                    limit,
  NostrDBIndexIterateCallback callback,
  void*                       user_data);

#endif
