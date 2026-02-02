#ifndef NOSTR_RESPONSE_H_
#define NOSTR_RESPONSE_H_

#include "../../util/types.h"
#include "../nostr_types.h"

// ============================================================================
// Generate EVENT response: ["EVENT", "<subscription_id>", <event>]
// ============================================================================
bool nostr_response_event(
  const char*             subscription_id,
  const NostrEventEntity* event,
  char*                   buffer,
  size_t                  capacity);

// ============================================================================
// Generate EOSE response: ["EOSE", "<subscription_id>"]
// ============================================================================
bool nostr_response_eose(
  const char* subscription_id,
  char*       buffer,
  size_t      capacity);

// ============================================================================
// Generate OK response: ["OK", "<event_id>", <success>, "<message>"]
// ============================================================================
bool nostr_response_ok(
  const char* event_id,
  bool        success,
  const char* message,
  char*       buffer,
  size_t      capacity);

// ============================================================================
// Generate NOTICE response: ["NOTICE", "<message>"]
// ============================================================================
bool nostr_response_notice(
  const char* message,
  char*       buffer,
  size_t      capacity);

// ============================================================================
// Generate CLOSED response: ["CLOSED", "<subscription_id>", "<message>"]
// ============================================================================
bool nostr_response_closed(
  const char* subscription_id,
  const char* message,
  char*       buffer,
  size_t      capacity);

#endif
