#include "../../../arch/memory.h"
#include "index_manager.h"

// ============================================================================
// index_kind_insert: Insert kind -> RecordId (duplicate key)
// ============================================================================
NostrDBError index_kind_insert(BTree* tree, uint32_t kind, RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);

  return btree_insert_dup(tree, &kind, rid);
}

// ============================================================================
// index_kind_delete: Delete kind -> RecordId from index
// ============================================================================
NostrDBError index_kind_delete(BTree* tree, uint32_t kind, RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);

  return btree_delete_dup(tree, &kind, rid);
}
