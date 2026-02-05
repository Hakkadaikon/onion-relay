#include "nostr_close.h"

#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"

// ============================================================================
// Initialize CLOSE message structure
// ============================================================================
void nostr_close_init(NostrCloseMessage* close_msg)
{
  if (close_msg == NULL) {
    return;
  }
  internal_memset(close_msg, 0, sizeof(NostrCloseMessage));
}

// ============================================================================
// Clear CLOSE message structure
// ============================================================================
void nostr_close_clear(NostrCloseMessage* close_msg)
{
  nostr_close_init(close_msg);
}

// ============================================================================
// Parse CLOSE message from JSON array
// ["CLOSE", <subscription_id>]
// ============================================================================
bool nostr_close_parse(
  const PJsonFuncs   funcs,
  const char*        json,
  const jsontok_t*   tokens,
  const size_t       token_count,
  NostrCloseMessage* close_msg)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(tokens, false);
  require_not_null(close_msg, false);

  nostr_close_init(close_msg);

  // Root must be an array
  if (!funcs->is_array(&tokens[0])) {
    log_debug("CLOSE error: not an array\n");
    return false;
  }

  int num_elements = tokens[0].size;
  if (num_elements < 2) {
    log_debug("CLOSE error: too few elements\n");
    return false;
  }

  // tokens[0] is the array
  // tokens[1] should be "CLOSE"
  // tokens[2] should be subscription_id

  if (token_count < 3) {
    log_debug("CLOSE error: not enough tokens\n");
    return false;
  }

  // Check "CLOSE" string
  const jsontok_t* close_token = &tokens[1];
  if (!funcs->is_string(close_token)) {
    log_debug("CLOSE error: first element is not a string\n");
    return false;
  }

  size_t close_len = funcs->get_token_length(close_token);
  if (close_len != 5) {
    log_debug("CLOSE error: not a CLOSE message\n");
    return false;
  }

  if (!strncmp(&json[close_token->start], "CLOSE", 5)) {
    log_debug("CLOSE error: not a CLOSE message\n");
    return false;
  }

  // Extract subscription_id
  const jsontok_t* sub_token = &tokens[2];
  if (!funcs->is_string(sub_token)) {
    log_debug("CLOSE error: subscription_id is not a string\n");
    return false;
  }

  size_t sub_len = funcs->get_token_length(sub_token);
  if (sub_len == 0) {
    log_debug("CLOSE error: subscription_id length invalid\n");
    return false;
  }

  if (sub_len > NOSTR_CLOSE_SUBSCRIPTION_ID_LENGTH) {
    log_debug("CLOSE error: subscription_id length invalid\n");
    return false;
  }

  internal_memcpy(close_msg->subscription_id, &json[sub_token->start], sub_len);
  close_msg->subscription_id[sub_len] = '\0';

  return true;
}
