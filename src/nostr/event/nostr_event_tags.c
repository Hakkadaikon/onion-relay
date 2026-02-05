#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

// ============================================================================
// Helper: Copy string from JSON token to buffer
// ============================================================================
static bool copy_token_string(
  const char*      json,
  const jsontok_t* token,
  char*            buffer,
  size_t           buffer_size)
{
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(buffer, false);
  require_valid_length(buffer_size, false);

  size_t len = (size_t)(token->end - token->start);
  if (len >= buffer_size) {
    len = buffer_size - 1;
  }

  internal_memcpy(buffer, &json[token->start], len);
  buffer[len] = '\0';

  return true;
}

// ============================================================================
// extract_nostr_event_tags
// Parses tags array: [["e", "event_id", "relay"], ["p", "pubkey"], ...]
// ============================================================================
bool extract_nostr_event_tags(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  NostrTagEntity*  tags,
  uint32_t*        tag_count)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(tags, false);
  require_not_null(tag_count, false);

  if (!funcs->is_array(token)) {
    log_debug("JSON error: tags is not array\n");
    return false;
  }

  // Get number of tags (outer array size)
  int num_tags = token->size;
  if (num_tags < 0) {
    log_debug("JSON error: invalid tags array size\n");
    return false;
  }

  // Initialize tag count
  *tag_count = 0;

  // Empty tags array is valid
  if (num_tags == 0) {
    return true;
  }

  // Token index starts after the outer array token
  int token_idx = 1;

  for (int tag_i = 0; tag_i < num_tags && *tag_count < NOSTR_EVENT_TAG_LENGTH; tag_i++) {
    const jsontok_t* tag_token = &token[token_idx];

    // Each tag should be an array
    if (!funcs->is_array(tag_token)) {
      log_debug("JSON error: tag element is not array\n");
      // Skip this malformed tag and try to continue
      token_idx++;
      continue;
    }

    int tag_size = tag_token->size;
    if (tag_size <= 0) {
      // Empty tag array, skip
      token_idx++;
      continue;
    }

    NostrTagEntity* current_tag = &tags[*tag_count];
    internal_memset(current_tag, 0, sizeof(NostrTagEntity));

    // Move to first element of tag array (the key)
    token_idx++;
    const jsontok_t* key_token = &token[token_idx];

    if (!funcs->is_string(key_token)) {
      log_debug("JSON error: tag key is not string\n");
      // Skip remaining tokens in this tag
      for (int j = 0; j < tag_size; j++) {
        token_idx++;
        // If the element is an array or object, skip its children too
        const jsontok_t* skip_token = &token[token_idx - 1];
        if (funcs->is_array(skip_token) || funcs->is_object(skip_token)) {
          // This is a simplified skip - might not handle deeply nested structures
        }
      }
      continue;
    }

    // Copy tag key
    copy_token_string(json, key_token, current_tag->key, sizeof(current_tag->key));
    token_idx++;

    // Copy tag values (remaining elements)
    current_tag->item_count = 0;
    for (int value_i = 1; value_i < tag_size && current_tag->item_count < NOSTR_EVENT_TAG_VALUE_COUNT; value_i++) {
      const jsontok_t* value_token = &token[token_idx];

      if (funcs->is_string(value_token)) {
        copy_token_string(
          json,
          value_token,
          current_tag->values[current_tag->item_count],
          NOSTR_EVENT_TAG_VALUE_LENGTH);
        current_tag->item_count++;
      }
      // Skip non-string values (shouldn't happen in valid Nostr events)
      token_idx++;
    }

    (*tag_count)++;
  }

  return true;
}
