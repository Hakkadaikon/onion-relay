#include "../../../arch/memory.h"
#include "index_manager.h"

// ============================================================================
// Internal: Build composite key tag_name[1] + tag_value[32] = 33 bytes
// ============================================================================
static void build_tag_key(uint8_t tag_name, const uint8_t tag_value[32],
                          uint8_t out[33])
{
  out[0] = tag_name;
  internal_memcpy(out + 1, tag_value, 32);
}

// ============================================================================
// index_tag_insert: Insert tag_name+tag_value -> RecordId (duplicate key)
// ============================================================================
NostrDBError index_tag_insert(BTree* tree, uint8_t tag_name,
                              const uint8_t tag_value[32], RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(tag_value, NOSTR_DB_ERROR_NULL_PARAM);

  uint8_t key[33];
  build_tag_key(tag_name, tag_value, key);
  return btree_insert_dup(tree, key, rid);
}

// ============================================================================
// index_tag_delete: Delete tag_name+tag_value -> RecordId from index
// ============================================================================
NostrDBError index_tag_delete(BTree* tree, uint8_t tag_name,
                              const uint8_t tag_value[32], RecordId rid)
{
  require_not_null(tree, NOSTR_DB_ERROR_NULL_PARAM);
  require_not_null(tag_value, NOSTR_DB_ERROR_NULL_PARAM);

  uint8_t key[33];
  build_tag_key(tag_name, tag_value, key);
  return btree_delete_dup(tree, key, rid);
}
