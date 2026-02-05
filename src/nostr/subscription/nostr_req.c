#include "nostr_req.h"

#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "nostr_filter.h"

// ============================================================================
// Count tokens in a JSON value
// ============================================================================
static size_t count_tokens_in_value(const jsontok_t* token, size_t remaining_tokens)
{
  if (remaining_tokens == 0) {
    return 0;
  }

  size_t count = 1;

  if (token->type == JSMN_OBJECT) {
    int    pairs = token->size;
    size_t idx   = 1;
    for (int i = 0; i < pairs && idx < remaining_tokens; i++) {
      idx += count_tokens_in_value(&token[idx], remaining_tokens - idx);
      if (idx < remaining_tokens) {
        idx += count_tokens_in_value(&token[idx], remaining_tokens - idx);
      }
    }
    count = idx;
  } else if (token->type == JSMN_ARRAY) {
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
// Initialize REQ message structure
// ============================================================================
void nostr_req_init(NostrReqMessage* req)
{
  if (req == NULL) {
    return;
  }
  internal_memset(req, 0, sizeof(NostrReqMessage));
}

// ============================================================================
// Clear REQ message structure
// ============================================================================
void nostr_req_clear(NostrReqMessage* req)
{
  nostr_req_init(req);
}

// ============================================================================
// Parse REQ message from JSON array
// ["REQ", <subscription_id>, <filter1>, <filter2>, ...]
// ============================================================================
bool nostr_req_parse(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* tokens,
  const size_t     token_count,
  NostrReqMessage* req)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(tokens, false);
  require_not_null(req, false);

  nostr_req_init(req);

  // Root must be an array
  if (!funcs->is_array(&tokens[0])) {
    log_debug("REQ error: not an array\n");
    return false;
  }

  int num_elements = tokens[0].size;
  if (num_elements < 3) {
    log_debug("REQ error: too few elements\n");
    return false;
  }

  // tokens[0] is the array
  // tokens[1] should be "REQ"
  // tokens[2] should be subscription_id
  // tokens[3...] are filters

  // Check "REQ" string
  const jsontok_t* req_token = &tokens[1];
  if (!funcs->is_string(req_token)) {
    log_debug("REQ error: first element is not a string\n");
    return false;
  }

  size_t req_len = funcs->get_token_length(req_token);
  if (req_len != 3) {
    log_debug("REQ error: not a REQ message\n");
    return false;
  }

  if (!strncmp(&json[req_token->start], "REQ", 3)) {
    log_debug("REQ error: not a REQ message\n");
    return false;
  }

  // Extract subscription_id
  const jsontok_t* sub_token = &tokens[2];
  if (!funcs->is_string(sub_token)) {
    log_debug("REQ error: subscription_id is not a string\n");
    return false;
  }

  size_t sub_len = funcs->get_token_length(sub_token);
  if (sub_len == 0) {
    log_debug("REQ error: subscription_id length invalid\n");
    return false;
  }

  if (sub_len > NOSTR_REQ_SUBSCRIPTION_ID_LENGTH) {
    log_debug("REQ error: subscription_id length invalid\n");
    return false;
  }

  internal_memcpy(req->subscription_id, &json[sub_token->start], sub_len);
  req->subscription_id[sub_len] = '\0';

  // Parse filters
  size_t token_idx = 3;
  for (int i = 2; i < num_elements && req->filters_count < NOSTR_REQ_MAX_FILTERS; i++) {
    if (token_idx >= token_count) {
      break;
    }

    const jsontok_t* filter_token = &tokens[token_idx];
    if (!funcs->is_object(filter_token)) {
      log_debug("REQ error: filter is not an object\n");
      token_idx += count_tokens_in_value(filter_token, token_count - token_idx);
      continue;
    }

    NostrFilter* filter = &req->filters[req->filters_count];
    if (nostr_filter_parse(funcs, json, filter_token, token_count - token_idx, filter)) {
      req->filters_count++;
    }

    token_idx += count_tokens_in_value(filter_token, token_count - token_idx);
  }

  return req->filters_count > 0;
}
