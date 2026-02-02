#include "nostr_filter.h"

#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"

// ============================================================================
// External declarations for field extractors
// ============================================================================
extern bool extract_nostr_filter_ids(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  NostrFilterId*   ids,
  size_t*          ids_count);

extern bool extract_nostr_filter_authors(
  const PJsonFuncs   funcs,
  const char*        json,
  const jsontok_t*   token,
  NostrFilterPubkey* authors,
  size_t*            authors_count);

extern bool extract_nostr_filter_kinds(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  uint32_t*        kinds,
  size_t*          kinds_count);

extern bool extract_nostr_filter_since(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  int64_t*         since);

extern bool extract_nostr_filter_until(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  int64_t*         until);

extern bool extract_nostr_filter_limit(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  uint32_t*        limit);

extern bool extract_nostr_filter_tag(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  char             tag_name,
  NostrFilterTag*  tag);

// ============================================================================
// Helper: Convert hex char to value
// ============================================================================
static int32_t hex_char_value(char c)
{
  if (is_digit(c)) {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

// ============================================================================
// Helper: Convert hex string to bytes
// ============================================================================
static size_t hex_to_bytes(const char* hex, size_t hex_len, uint8_t* out, size_t out_capacity)
{
  size_t pairs = hex_len / 2;
  if (pairs > out_capacity) {
    pairs = out_capacity;
  }

  for (size_t i = 0; i < pairs; i++) {
    int32_t h = hex_char_value(hex[i * 2]);
    int32_t l = hex_char_value(hex[i * 2 + 1]);

    if (h < 0 || l < 0) {
      return i;
    }

    out[i] = (uint8_t)((h << 4) | l);
  }

  return pairs;
}

// ============================================================================
// Initialize filter to default values
// ============================================================================
void nostr_filter_init(NostrFilter* filter)
{
  if (filter == NULL) {
    return;
  }
  internal_memset(filter, 0, sizeof(NostrFilter));
}

// ============================================================================
// Clear filter (same as init)
// ============================================================================
void nostr_filter_clear(NostrFilter* filter)
{
  nostr_filter_init(filter);
}

// ============================================================================
// Count tokens in a JSON object child
// ============================================================================
static size_t count_tokens_in_value(const jsontok_t* token, size_t remaining_tokens)
{
  if (remaining_tokens == 0) {
    return 0;
  }

  size_t count = 1;  // Count this token

  if (token->type == JSMN_OBJECT) {
    // Count children (key-value pairs)
    int    pairs = token->size;
    size_t idx   = 1;
    for (int i = 0; i < pairs && idx < remaining_tokens; i++) {
      // Skip key
      idx += count_tokens_in_value(&token[idx], remaining_tokens - idx);
      if (idx < remaining_tokens) {
        // Skip value
        idx += count_tokens_in_value(&token[idx], remaining_tokens - idx);
      }
    }
    count = idx;
  } else if (token->type == JSMN_ARRAY) {
    // Count array elements
    int    elems = token->size;
    size_t idx   = 1;
    for (int i = 0; i < elems && idx < remaining_tokens; i++) {
      idx += count_tokens_in_value(&token[idx], remaining_tokens - idx);
    }
    count = idx;
  }

  return count;
}

// ============================================================================
// Parse filter from JSON object
// ============================================================================
bool nostr_filter_parse(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  const size_t     token_count,
  NostrFilter*     filter)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(filter, false);

  if (!funcs->is_object(token)) {
    log_debug("Filter error: not an object\n");
    return false;
  }

  nostr_filter_init(filter);

  int num_fields = token->size;
  if (num_fields < 0) {
    log_debug("Filter error: invalid object size\n");
    return false;
  }

  if (num_fields == 0) {
    return true;
  }

  size_t token_idx = 1;  // Skip object token

  for (int i = 0; i < num_fields && token_idx < token_count; i++) {
    const jsontok_t* key_token = &token[token_idx];
    if (!funcs->is_string(key_token)) {
      log_debug("Filter error: key is not a string\n");
      return false;
    }

    token_idx++;
    if (token_idx >= token_count) {
      break;
    }

    const jsontok_t* val_token = &token[token_idx];
    size_t           key_len   = funcs->get_token_length(key_token);
    const char*      key_str   = &json[key_token->start];

    // Parse known fields
    if (key_len == 3 && strncmp(key_str, "ids", 3)) {
      extract_nostr_filter_ids(funcs, json, val_token, filter->ids, &filter->ids_count);
    } else if (key_len == 7 && strncmp(key_str, "authors", 7)) {
      extract_nostr_filter_authors(funcs, json, val_token, filter->authors, &filter->authors_count);
    } else if (key_len == 5 && strncmp(key_str, "kinds", 5)) {
      extract_nostr_filter_kinds(funcs, json, val_token, filter->kinds, &filter->kinds_count);
    } else if (key_len == 5 && strncmp(key_str, "since", 5)) {
      extract_nostr_filter_since(funcs, json, val_token, &filter->since);
    } else if (key_len == 5 && strncmp(key_str, "until", 5)) {
      extract_nostr_filter_until(funcs, json, val_token, &filter->until);
    } else if (key_len == 5 && strncmp(key_str, "limit", 5)) {
      extract_nostr_filter_limit(funcs, json, val_token, &filter->limit);
    } else if (key_len == 2 && key_str[0] == '#' && filter->tags_count < NOSTR_FILTER_MAX_TAGS) {
      // Tag filter like "#e", "#p", "#t"
      char tag_name = key_str[1];
      extract_nostr_filter_tag(funcs, json, val_token, tag_name, &filter->tags[filter->tags_count]);
      if (filter->tags[filter->tags_count].values_count > 0) {
        filter->tags_count++;
      }
    }

    // Skip value tokens
    token_idx += count_tokens_in_value(val_token, token_count - token_idx);
  }

  return true;
}

// ============================================================================
// Helper: Check if bytes match with prefix
// ============================================================================
static bool bytes_match_prefix(const uint8_t* data, const uint8_t* prefix, size_t prefix_len)
{
  for (size_t i = 0; i < prefix_len; i++) {
    if (data[i] != prefix[i]) {
      return false;
    }
  }
  return true;
}

// ============================================================================
// Check if event matches filter
// ============================================================================
bool nostr_filter_matches(
  const NostrFilter*      filter,
  const NostrEventEntity* event)
{
  require_not_null(filter, false);
  require_not_null(event, false);

  // Check ids filter (prefix match supported)
  if (filter->ids_count > 0) {
    bool    found = false;
    uint8_t event_id_bin[32];
    hex_to_bytes(event->id, 64, event_id_bin, 32);

    for (size_t i = 0; i < filter->ids_count; i++) {
      if (bytes_match_prefix(event_id_bin, filter->ids[i].value, filter->ids[i].prefix_len)) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }

  // Check authors filter (prefix match supported)
  if (filter->authors_count > 0) {
    bool    found = false;
    uint8_t event_pubkey_bin[32];
    hex_to_bytes(event->pubkey, 64, event_pubkey_bin, 32);

    for (size_t i = 0; i < filter->authors_count; i++) {
      if (bytes_match_prefix(event_pubkey_bin, filter->authors[i].value, filter->authors[i].prefix_len)) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }

  // Check kinds filter
  if (filter->kinds_count > 0) {
    bool found = false;
    for (size_t i = 0; i < filter->kinds_count; i++) {
      if (filter->kinds[i] == event->kind) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }

  // Check since (event created_at >= since)
  if (filter->since > 0) {
    if ((int64_t)event->created_at < filter->since) {
      return false;
    }
  }

  // Check until (event created_at <= until)
  if (filter->until > 0) {
    if ((int64_t)event->created_at > filter->until) {
      return false;
    }
  }

  // Check tag filters
  for (size_t ti = 0; ti < filter->tags_count; ti++) {
    const NostrFilterTag* ftag  = &filter->tags[ti];
    bool                  found = false;

    // Search in event tags
    for (uint32_t ei = 0; ei < event->tag_count && !found; ei++) {
      const NostrTagEntity* etag = &event->tags[ei];

      // Check if tag name matches (key[0] is the tag letter)
      if (etag->key[0] != ftag->name || etag->key[1] != '\0') {
        continue;
      }

      // Check if any value matches (values[0] is the first value after the tag name)
      if (etag->item_count < 1) {
        continue;
      }

      for (size_t fvi = 0; fvi < ftag->values_count && !found; fvi++) {
        for (size_t evi = 0; evi < etag->item_count && !found; evi++) {
          const char* eval     = etag->values[evi];
          size_t      eval_len = strlen(eval);

          // For 'e' and 'p' tags, compare binary
          if (ftag->name == 'e' || ftag->name == 'p') {
            if (eval_len == 64) {
              uint8_t eval_bin[32];
              hex_to_bytes(eval, 64, eval_bin, 32);
              if (internal_memcmp(eval_bin, ftag->values[fvi], 32) == 0) {
                found = true;
              }
            }
          } else {
            // For other tags, compare as strings
            size_t fval_len = strlen((const char*)ftag->values[fvi]);
            if (eval_len == fval_len) {
              if (strncmp(eval, (const char*)ftag->values[fvi], fval_len)) {
                found = true;
              }
            }
          }
        }
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}
