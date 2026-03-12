#ifndef NOSTR_DB_INDEX_MANAGER_H_
#define NOSTR_DB_INDEX_MANAGER_H_

#include "../../../util/types.h"
#include "../btree/btree.h"
#include "../btree/btree_types.h"
#include "../buffer/buffer_pool.h"
#include "../db_types.h"
#include "../record/record_types.h"

// ============================================================================
// IndexManager: unified management of all Nostr indexes
// ============================================================================
typedef struct {
  BTree       id_index;           // Unique: id[32] -> RecordId
  BTree       timeline_index;     // Dup: INT64_MAX - created_at -> overflow
  BTree       pubkey_index;       // Dup: pubkey[32] -> overflow
  BTree       kind_index;         // Dup: kind (uint32) -> overflow
  BTree       pubkey_kind_index;  // Dup: pubkey[32]+kind[4] -> overflow
  BTree       tag_index;          // Dup: tag_name[1]+tag_value[32] -> overflow
  BufferPool* pool;
} IndexManager;

// ============================================================================
// Initialization
// ============================================================================

/**
 * @brief Create all indexes (first-time initialization)
 */
NostrDBError index_manager_create(IndexManager* im, BufferPool* pool);

/**
 * @brief Open existing indexes from their meta pages
 */
NostrDBError index_manager_open(IndexManager* im, BufferPool* pool,
                                page_id_t id_meta, page_id_t timeline_meta,
                                page_id_t pubkey_meta, page_id_t kind_meta,
                                page_id_t pk_kind_meta, page_id_t tag_meta);

/**
 * @brief Flush all index metadata to disk
 */
NostrDBError index_manager_flush(IndexManager* im);

// ============================================================================
// Bulk operations (insert/delete event across all indexes)
// ============================================================================

/**
 * @brief Insert an event into all indexes
 * @param im IndexManager
 * @param rid RecordId of the stored event record
 * @param record Pointer to EventRecord (header only needed for indexes)
 * @param tags_data Serialized tags data (from after EventRecord header + content)
 * @param tags_length Length of serialized tags data
 */
NostrDBError index_manager_insert_event(IndexManager* im, RecordId rid,
                                        const EventRecord* record,
                                        const uint8_t*     tags_data,
                                        uint16_t           tags_length);

/**
 * @brief Delete an event from all indexes
 */
NostrDBError index_manager_delete_event(IndexManager* im, RecordId rid,
                                        const EventRecord* record,
                                        const uint8_t*     tags_data,
                                        uint16_t           tags_length);

// ============================================================================
// ID index operations (unique key)
// ============================================================================

NostrDBError index_id_insert(BTree* tree, const uint8_t id[32], RecordId rid);
NostrDBError index_id_lookup(BTree* tree, const uint8_t id[32],
                             RecordId* out_rid);
NostrDBError index_id_delete(BTree* tree, const uint8_t id[32]);

// ============================================================================
// Timeline index operations (duplicate key, descending order)
// ============================================================================

/**
 * @brief Encode created_at for descending order in B+ tree
 */
static inline int64_t timeline_key_encode(int64_t created_at)
{
  return INT64_MAX - created_at;
}

static inline int64_t timeline_key_decode(int64_t encoded)
{
  return INT64_MAX - encoded;
}

NostrDBError index_timeline_insert(BTree* tree, int64_t created_at,
                                   RecordId rid);
NostrDBError index_timeline_delete(BTree* tree, int64_t created_at,
                                   RecordId rid);

// ============================================================================
// Pubkey index operations (duplicate key)
// ============================================================================

NostrDBError index_pubkey_insert(BTree* tree, const uint8_t pubkey[32],
                                 RecordId rid);
NostrDBError index_pubkey_delete(BTree* tree, const uint8_t pubkey[32],
                                 RecordId rid);

// ============================================================================
// Kind index operations (duplicate key)
// ============================================================================

NostrDBError index_kind_insert(BTree* tree, uint32_t kind, RecordId rid);
NostrDBError index_kind_delete(BTree* tree, uint32_t kind, RecordId rid);

// ============================================================================
// Pubkey+Kind composite index operations (duplicate key)
// ============================================================================

NostrDBError index_pk_kind_insert(BTree* tree, const uint8_t pubkey[32],
                                  uint32_t kind, RecordId rid);
NostrDBError index_pk_kind_delete(BTree* tree, const uint8_t pubkey[32],
                                  uint32_t kind, RecordId rid);

// ============================================================================
// Tag index operations (duplicate key)
// ============================================================================

NostrDBError index_tag_insert(BTree* tree, uint8_t tag_name,
                              const uint8_t tag_value[32], RecordId rid);
NostrDBError index_tag_delete(BTree* tree, uint8_t tag_name,
                              const uint8_t tag_value[32], RecordId rid);

#endif
