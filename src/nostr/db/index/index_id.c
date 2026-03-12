#include "../../../arch/memory.h"
#include "index_manager.h"

// ============================================================================
// index_id_insert: Insert event ID -> RecordId (unique)
// ============================================================================
NostrDBError index_id_insert(BTree* tree, const uint8_t id[32], RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(id, NOSTR_DB_ERROR_NULL_PARAM);

  return btree_insert(tree, id, &rid);
}

// ============================================================================
// index_id_lookup: Look up RecordId by event ID
// ============================================================================
NostrDBError index_id_lookup(BTree* tree, const uint8_t id[32],
                             RecordId* out_rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(id, NOSTR_DB_ERROR_NULL_PARAM);

  return btree_search(tree, id, out_rid);
}

// ============================================================================
// index_id_delete: Delete event ID from index
// ============================================================================
NostrDBError index_id_delete(BTree* tree, const uint8_t id[32])
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(id, NOSTR_DB_ERROR_NULL_PARAM);

  return btree_delete(tree, id);
}
