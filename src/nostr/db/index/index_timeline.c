#include "../../../arch/memory.h"
#include "index_manager.h"

// ============================================================================
// index_timeline_insert: Insert created_at -> RecordId (duplicate key)
// Key is encoded as INT64_MAX - created_at for descending order
// ============================================================================
NostrDBError index_timeline_insert(BTree* tree, int64_t created_at,
                                   RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);

  int64_t key = timeline_key_encode(created_at);
  return btree_insert_dup(tree, &key, rid);
}

// ============================================================================
// index_timeline_delete: Delete created_at -> RecordId from timeline
// ============================================================================
NostrDBError index_timeline_delete(BTree* tree, int64_t created_at,
                                   RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);

  int64_t key = timeline_key_encode(created_at);
  return btree_delete_dup(tree, &key, rid);
}
