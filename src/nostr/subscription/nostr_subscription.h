#ifndef NOSTR_SUBSCRIPTION_H_
#define NOSTR_SUBSCRIPTION_H_

#include "../../util/types.h"
#include "../nostr_types.h"
#include "nostr_filter_types.h"
#include "nostr_req.h"

// ============================================================================
// Constants
// ============================================================================
#define NOSTR_SUBSCRIPTION_MAX_COUNT 256

// ============================================================================
// Subscription entry
// ============================================================================
typedef struct {
  bool        active;
  int32_t     client_fd;  // Associated client socket
  char        subscription_id[NOSTR_REQ_SUBSCRIPTION_ID_LENGTH + 1];
  NostrFilter filters[NOSTR_REQ_MAX_FILTERS];
  size_t      filters_count;
} NostrSubscription, *PNostrSubscription;

// ============================================================================
// Subscription manager
// ============================================================================
typedef struct {
  NostrSubscription subscriptions[NOSTR_SUBSCRIPTION_MAX_COUNT];
  size_t            count;
} NostrSubscriptionManager, *PNostrSubscriptionManager;

// ============================================================================
// Initialize subscription manager
// ============================================================================
void nostr_subscription_manager_init(NostrSubscriptionManager* manager);

// ============================================================================
// Add a new subscription
// Returns pointer to the subscription, or NULL on failure
// ============================================================================
NostrSubscription* nostr_subscription_add(
  NostrSubscriptionManager* manager,
  int32_t                   client_fd,
  const NostrReqMessage*    req);

// ============================================================================
// Remove a subscription by subscription_id and client_fd
// Returns true if subscription was found and removed
// ============================================================================
bool nostr_subscription_remove(
  NostrSubscriptionManager* manager,
  int32_t                   client_fd,
  const char*               subscription_id);

// ============================================================================
// Remove all subscriptions for a client
// Returns number of subscriptions removed
// ============================================================================
size_t nostr_subscription_remove_client(
  NostrSubscriptionManager* manager,
  int32_t                   client_fd);

// ============================================================================
// Find a subscription by subscription_id and client_fd
// Returns pointer to the subscription, or NULL if not found
// ============================================================================
NostrSubscription* nostr_subscription_find(
  NostrSubscriptionManager* manager,
  int32_t                   client_fd,
  const char*               subscription_id);

// ============================================================================
// Check if an event matches any filter in a subscription
// ============================================================================
bool nostr_subscription_matches_event(
  const NostrSubscription* subscription,
  const NostrEventEntity*  event);

// ============================================================================
// Iterate over all active subscriptions that match an event
// Callback is called for each matching subscription
// Returns number of matching subscriptions
// ============================================================================
typedef void (*NostrSubscriptionMatchCallback)(
  const NostrSubscription* subscription,
  void*                    user_data);

size_t nostr_subscription_find_matching(
  NostrSubscriptionManager*      manager,
  const NostrEventEntity*        event,
  NostrSubscriptionMatchCallback callback,
  void*                          user_data);

#endif
