#ifndef NOSTR_CLOSE_H_
#define NOSTR_CLOSE_H_

#define JSMN_HEADER
#include "../../json/json_wrapper.h"

// ============================================================================
// Constants
// ============================================================================
#define NOSTR_CLOSE_SUBSCRIPTION_ID_LENGTH 64

// ============================================================================
// CLOSE message structure
// ============================================================================
typedef struct {
  char subscription_id[NOSTR_CLOSE_SUBSCRIPTION_ID_LENGTH + 1];
} NostrCloseMessage, *PNostrCloseMessage;

// ============================================================================
// Initialize CLOSE message structure
// ============================================================================
void nostr_close_init(NostrCloseMessage* close_msg);

// ============================================================================
// Parse CLOSE message from JSON array
// ["CLOSE", <subscription_id>]
// ============================================================================
bool nostr_close_parse(
  const PJsonFuncs   funcs,
  const char*        json,
  const jsontok_t*   tokens,
  const size_t       token_count,
  NostrCloseMessage* close_msg);

// ============================================================================
// Clear CLOSE message structure
// ============================================================================
void nostr_close_clear(NostrCloseMessage* close_msg);

#endif
