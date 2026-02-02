#ifndef NOSTR_REQ_H_
#define NOSTR_REQ_H_

#define JSMN_HEADER
#include "../../json/json_wrapper.h"
#include "nostr_filter_types.h"

// ============================================================================
// Constants
// ============================================================================
#define NOSTR_REQ_SUBSCRIPTION_ID_LENGTH 64
#define NOSTR_REQ_MAX_FILTERS 16

// ============================================================================
// REQ message structure
// ============================================================================
typedef struct {
  char        subscription_id[NOSTR_REQ_SUBSCRIPTION_ID_LENGTH + 1];
  NostrFilter filters[NOSTR_REQ_MAX_FILTERS];
  size_t      filters_count;
} NostrReqMessage, *PNostrReqMessage;

// ============================================================================
// Initialize REQ message structure
// ============================================================================
void nostr_req_init(NostrReqMessage* req);

// ============================================================================
// Parse REQ message from JSON array
// ["REQ", <subscription_id>, <filter1>, <filter2>, ...]
// ============================================================================
bool nostr_req_parse(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* tokens,
  const size_t     token_count,
  NostrReqMessage* req);

// ============================================================================
// Clear REQ message structure
// ============================================================================
void nostr_req_clear(NostrReqMessage* req);

#endif
