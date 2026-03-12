#include "../../../arch/memory.h"
#include "index_manager.h"

// ============================================================================
// Internal: Build composite key pubkey[32] + kind[4] = 36 bytes
// ============================================================================
static void build_pk_kind_key(const uint8_t pubkey[32], uint32_t kind,
                              uint8_t out[36])
{
  internal_memcpy(out, pubkey, 32);
  internal_memcpy(out + 32, &kind, sizeof(uint32_t));
}

// ============================================================================
// index_pk_kind_insert: Insert pubkey+kind -> RecordId (duplicate key)
// ============================================================================
NostrDBError index_pk_kind_insert(BTree* tree, const uint8_t pubkey[32],
                                  uint32_t kind, RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(pubkey, NOSTR_DB_ERROR_NULL_PARAM);

  uint8_t key[36];
  build_pk_kind_key(pubkey, kind, key);
  return btree_insert_dup(tree, key, rid);
}

// ============================================================================
// index_pk_kind_delete: Delete pubkey+kind -> RecordId from index
// ============================================================================
NostrDBError index_pk_kind_delete(BTree* tree, const uint8_t pubkey[32],
                                  uint32_t kind, RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(pubkey, NOSTR_DB_ERROR_NULL_PARAM);

  uint8_t key[36];
  build_pk_kind_key(pubkey, kind, key);
  return btree_delete_dup(tree, key, rid);
}
