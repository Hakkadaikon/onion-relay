#include "../../../arch/memory.h"
#include "index_manager.h"

// ============================================================================
// index_pubkey_insert: Insert pubkey -> RecordId (duplicate key)
// ============================================================================
NostrDBError index_pubkey_insert(BTree* tree, const uint8_t pubkey[32],
                                 RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(pubkey, NOSTR_DB_ERROR_NULL_PARAM);

  return btree_insert_dup(tree, pubkey, rid);
}

// ============================================================================
// index_pubkey_delete: Delete pubkey -> RecordId from index
// ============================================================================
NostrDBError index_pubkey_delete(BTree* tree, const uint8_t pubkey[32],
                                 RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(pubkey, NOSTR_DB_ERROR_NULL_PARAM);

  return btree_delete_dup(tree, pubkey, rid);
}
