#ifndef NOSTR_FILTER_TYPES_H_
#define NOSTR_FILTER_TYPES_H_

#include "../../util/types.h"

// ============================================================================
// Constants
// ============================================================================
#define NOSTR_FILTER_MAX_IDS 256
#define NOSTR_FILTER_MAX_AUTHORS 256
#define NOSTR_FILTER_MAX_KINDS 64
#define NOSTR_FILTER_MAX_TAGS 26
#define NOSTR_FILTER_MAX_TAG_VALUES 256
#define NOSTR_FILTER_ID_LENGTH 32
#define NOSTR_FILTER_PUBKEY_LENGTH 32
#define NOSTR_FILTER_TAG_VALUE_LENGTH 256

// ============================================================================
// Filter ID (supports prefix matching)
// ============================================================================
typedef struct {
  uint8_t value[NOSTR_FILTER_ID_LENGTH];
  size_t  prefix_len;  // 0 = full match, >0 = prefix match length in bytes
} NostrFilterId;

// ============================================================================
// Filter Pubkey (supports prefix matching)
// ============================================================================
typedef struct {
  uint8_t value[NOSTR_FILTER_PUBKEY_LENGTH];
  size_t  prefix_len;
} NostrFilterPubkey;

// ============================================================================
// Filter Tag
// ============================================================================
typedef struct {
  char    name;  // Tag name character ('e', 'p', 't', etc.)
  uint8_t values[NOSTR_FILTER_MAX_TAG_VALUES][32];
  size_t  values_count;
} NostrFilterTag;

// ============================================================================
// NostrFilter - Complete filter structure
// ============================================================================
typedef struct {
  // ids filter
  NostrFilterId ids[NOSTR_FILTER_MAX_IDS];
  size_t        ids_count;

  // authors filter
  NostrFilterPubkey authors[NOSTR_FILTER_MAX_AUTHORS];
  size_t            authors_count;

  // kinds filter
  uint32_t kinds[NOSTR_FILTER_MAX_KINDS];
  size_t   kinds_count;

  // tag filters (#e, #p, #t, etc.)
  NostrFilterTag tags[NOSTR_FILTER_MAX_TAGS];
  size_t         tags_count;

  // time range
  int64_t since;  // 0 = no constraint
  int64_t until;  // 0 = no constraint

  // result limit
  uint32_t limit;  // 0 = use default
} NostrFilter, *PNostrFilter;

#endif
