#ifndef NOSTR_FILTER_H_
#define NOSTR_FILTER_H_

#define JSMN_HEADER
#include "../../json/json_wrapper.h"
#include "../nostr_types.h"
#include "nostr_filter_types.h"

// ============================================================================
// Filter initialization (zero-fill)
// ============================================================================
void nostr_filter_init(NostrFilter* filter);

// ============================================================================
// Parse filter from JSON object
// ============================================================================
bool nostr_filter_parse(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  const size_t     token_count,
  NostrFilter*     filter);

// ============================================================================
// Check if event matches filter
// ============================================================================
bool nostr_filter_matches(
  const NostrFilter*      filter,
  const NostrEventEntity* event);

// ============================================================================
// Clear filter (same as init)
// ============================================================================
void nostr_filter_clear(NostrFilter* filter);

#endif
