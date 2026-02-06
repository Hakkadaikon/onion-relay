#include "nostr_subscription.h"

#include "../../arch/memory.h"
#include "../../arch/mmap.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "nostr_filter.h"

// ============================================================================
// Initialize subscription manager (allocates subscriptions via mmap)
// ============================================================================
bool nostr_subscription_manager_init(NostrSubscriptionManager* manager)
{
  if (manager == NULL) {
    return false;
  }

  size_t alloc_size = sizeof(NostrSubscription) * NOSTR_SUBSCRIPTION_MAX_COUNT;
  void*  ptr        = internal_mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    log_error("Failed to allocate subscription manager\n");
    manager->subscriptions = NULL;
    manager->count         = 0;
    return false;
  }

  manager->subscriptions = (NostrSubscription*)ptr;
  manager->count         = 0;
  return true;
}

// ============================================================================
// Destroy subscription manager (frees subscriptions via munmap)
// ============================================================================
void nostr_subscription_manager_destroy(NostrSubscriptionManager* manager)
{
  if (manager == NULL) {
    return;
  }

  if (manager->subscriptions != NULL) {
    size_t alloc_size = sizeof(NostrSubscription) * NOSTR_SUBSCRIPTION_MAX_COUNT;
    internal_munmap(manager->subscriptions, alloc_size);
    manager->subscriptions = NULL;
  }

  manager->count = 0;
}

// ============================================================================
// Add a new subscription
// ============================================================================
NostrSubscription* nostr_subscription_add(
  NostrSubscriptionManager* manager,
  int32_t                   client_fd,
  const NostrReqMessage*    req)
{
  require_not_null(manager, NULL);
  require_not_null(req, NULL);

  // Check if subscription already exists (update it)
  for (size_t i = 0; i < NOSTR_SUBSCRIPTION_MAX_COUNT; i++) {
    NostrSubscription* sub = &manager->subscriptions[i];
    if (sub->active && sub->client_fd == client_fd) {
      size_t sub_id_len = strlen(sub->subscription_id);
      size_t req_id_len = strlen(req->subscription_id);
      if (sub_id_len == req_id_len && strncmp(sub->subscription_id, req->subscription_id, sub_id_len)) {
        // Update existing subscription
        internal_memcpy(sub->filters, req->filters, sizeof(NostrFilter) * req->filters_count);
        sub->filters_count = req->filters_count;
        return sub;
      }
    }
  }

  // Find an empty slot
  for (size_t i = 0; i < NOSTR_SUBSCRIPTION_MAX_COUNT; i++) {
    NostrSubscription* sub = &manager->subscriptions[i];
    if (!sub->active) {
      sub->active    = true;
      sub->client_fd = client_fd;
      internal_memcpy(sub->subscription_id, req->subscription_id, strlen(req->subscription_id) + 1);
      internal_memcpy(sub->filters, req->filters, sizeof(NostrFilter) * req->filters_count);
      sub->filters_count = req->filters_count;
      manager->count++;
      return sub;
    }
  }

  log_debug("Subscription manager: no free slots\n");
  return NULL;
}

// ============================================================================
// Remove a subscription by subscription_id and client_fd
// ============================================================================
bool nostr_subscription_remove(
  NostrSubscriptionManager* manager,
  int32_t                   client_fd,
  const char*               subscription_id)
{
  require_not_null(manager, false);
  require_not_null(subscription_id, false);

  size_t search_id_len = strlen(subscription_id);

  for (size_t i = 0; i < NOSTR_SUBSCRIPTION_MAX_COUNT; i++) {
    NostrSubscription* sub = &manager->subscriptions[i];
    if (sub->active && sub->client_fd == client_fd) {
      size_t sub_id_len = strlen(sub->subscription_id);
      if (sub_id_len == search_id_len && strncmp(sub->subscription_id, subscription_id, sub_id_len)) {
        internal_memset(sub, 0, sizeof(NostrSubscription));
        manager->count--;
        return true;
      }
    }
  }

  return false;
}

// ============================================================================
// Remove all subscriptions for a client
// ============================================================================
size_t nostr_subscription_remove_client(
  NostrSubscriptionManager* manager,
  int32_t                   client_fd)
{
  require_not_null(manager, 0);

  size_t removed = 0;
  for (size_t i = 0; i < NOSTR_SUBSCRIPTION_MAX_COUNT; i++) {
    NostrSubscription* sub = &manager->subscriptions[i];
    if (sub->active && sub->client_fd == client_fd) {
      internal_memset(sub, 0, sizeof(NostrSubscription));
      manager->count--;
      removed++;
    }
  }

  return removed;
}

// ============================================================================
// Find a subscription by subscription_id and client_fd
// ============================================================================
NostrSubscription* nostr_subscription_find(
  NostrSubscriptionManager* manager,
  int32_t                   client_fd,
  const char*               subscription_id)
{
  require_not_null(manager, NULL);
  require_not_null(subscription_id, NULL);

  size_t search_id_len = strlen(subscription_id);

  for (size_t i = 0; i < NOSTR_SUBSCRIPTION_MAX_COUNT; i++) {
    NostrSubscription* sub = &manager->subscriptions[i];
    if (sub->active && sub->client_fd == client_fd) {
      size_t sub_id_len = strlen(sub->subscription_id);
      if (sub_id_len == search_id_len && strncmp(sub->subscription_id, subscription_id, sub_id_len)) {
        return sub;
      }
    }
  }

  return NULL;
}

// ============================================================================
// Check if an event matches any filter in a subscription
// ============================================================================
bool nostr_subscription_matches_event(
  const NostrSubscription* subscription,
  const NostrEventEntity*  event)
{
  require_not_null(subscription, false);
  require_not_null(event, false);

  if (!subscription->active) {
    return false;
  }

  // An event matches if it matches ANY filter in the subscription
  for (size_t i = 0; i < subscription->filters_count; i++) {
    if (nostr_filter_matches(&subscription->filters[i], event)) {
      return true;
    }
  }

  return false;
}

// ============================================================================
// Iterate over all active subscriptions that match an event
// ============================================================================
size_t nostr_subscription_find_matching(
  NostrSubscriptionManager*      manager,
  const NostrEventEntity*        event,
  NostrSubscriptionMatchCallback callback,
  void*                          user_data)
{
  require_not_null(manager, 0);
  require_not_null(event, 0);

  size_t count = 0;

  for (size_t i = 0; i < NOSTR_SUBSCRIPTION_MAX_COUNT; i++) {
    NostrSubscription* sub = &manager->subscriptions[i];
    if (sub->active && nostr_subscription_matches_event(sub, event)) {
      count++;
      if (callback != NULL) {
        callback(sub, user_data);
      }
    }
  }

  return count;
}
