#ifndef NOSTR_DB_INDEX_TYPES_H_
#define NOSTR_DB_INDEX_TYPES_H_

#include "../../../util/types.h"
#include "../db_types.h"

// ============================================================================
// ID Index structures
// ============================================================================

/**
 * ID Index bucket (48 bytes)
 * Open addressing hash table entry
 */
typedef struct {
  uint8_t  id[32];        // Event ID (raw bytes)
  uint64_t event_offset;  // Offset in events.dat
  uint8_t  state;         // NOSTR_DB_BUCKET_STATE_*
  uint8_t  padding[7];
} NostrDBIdBucket;

_Static_assert(sizeof(NostrDBIdBucket) == 48, "NostrDBIdBucket must be 48 bytes");

// ============================================================================
// Pubkey Index structures
// ============================================================================

/**
 * Pubkey Index bucket (48 bytes)
 * Hash table entry pointing to linked list of events
 */
typedef struct {
  uint8_t  pubkey[32];           // Public key (raw bytes)
  uint64_t newest_entry_offset;  // Offset to newest entry in pool
  uint32_t event_count;          // Number of events for this pubkey
  uint8_t  state;                // NOSTR_DB_BUCKET_STATE_*
  uint8_t  padding[3];
} NostrDBPubkeyBucket;

_Static_assert(sizeof(NostrDBPubkeyBucket) == 48, "NostrDBPubkeyBucket must be 48 bytes");

/**
 * Pubkey Index entry (24 bytes)
 * Linked list entry in entry pool
 */
typedef struct {
  uint64_t event_offset;        // Offset in events.dat
  int64_t  created_at;          // Timestamp for ordering
  uint64_t older_entry_offset;  // Next (older) entry offset, 0 = end
} NostrDBPubkeyEntry;

_Static_assert(sizeof(NostrDBPubkeyEntry) == 24, "NostrDBPubkeyEntry must be 24 bytes");

// ============================================================================
// Kind Index structures
// ============================================================================

/**
 * Kind Index slot (16 bytes)
 * Direct array indexed by kind (0-65535)
 */
typedef struct {
  uint64_t newest_entry_offset;  // Offset to newest entry in pool
  uint64_t event_count;          // Number of events for this kind
} NostrDBKindSlot;

_Static_assert(sizeof(NostrDBKindSlot) == 16, "NostrDBKindSlot must be 16 bytes");

/**
 * Kind Index entry (24 bytes)
 * Linked list entry in entry pool (same as pubkey entry)
 */
typedef struct {
  uint64_t event_offset;        // Offset in events.dat
  int64_t  created_at;          // Timestamp for ordering
  uint64_t older_entry_offset;  // Next (older) entry offset, 0 = end
} NostrDBKindEntry;

_Static_assert(sizeof(NostrDBKindEntry) == 24, "NostrDBKindEntry must be 24 bytes");

// ============================================================================
// Pubkey+Kind composite Index structures
// ============================================================================

/**
 * Pubkey+Kind Index bucket (56 bytes)
 */
typedef struct {
  uint8_t  pubkey[32];           // Public key
  uint32_t kind;                 // Event kind
  uint64_t newest_entry_offset;  // Offset to newest entry
  uint32_t event_count;          // Number of events
  uint8_t  state;                // NOSTR_DB_BUCKET_STATE_*
  uint8_t  padding[3];
} NostrDBPubkeyKindBucket;

_Static_assert(sizeof(NostrDBPubkeyKindBucket) == 56, "NostrDBPubkeyKindBucket must be 56 bytes");

// ============================================================================
// Tag Index structures
// ============================================================================

/**
 * Tag Index bucket (56 bytes)
 */
typedef struct {
  uint8_t  tag_name;             // Tag name character ('e', 'p', etc.)
  uint8_t  tag_value[32];        // Tag value (first 32 bytes)
  uint8_t  padding1[7];          // Padding for alignment
  uint64_t newest_entry_offset;  // Offset to newest entry
  uint32_t event_count;          // Number of events
  uint8_t  state;                // NOSTR_DB_BUCKET_STATE_*
  uint8_t  padding2[3];
} NostrDBTagBucket;

_Static_assert(sizeof(NostrDBTagBucket) == 56, "NostrDBTagBucket must be 56 bytes");

// ============================================================================
// Timeline Index structures
// ============================================================================

/**
 * Timeline Index entry (16 bytes)
 * Sorted array by created_at
 */
typedef struct {
  int64_t  created_at;    // Timestamp
  uint64_t event_offset;  // Offset in events.dat
} NostrDBTimelineEntry;

_Static_assert(sizeof(NostrDBTimelineEntry) == 16, "NostrDBTimelineEntry must be 16 bytes");

// ============================================================================
// Common index context structure
// ============================================================================

/**
 * Index context for passing to index functions
 */
typedef struct {
  void*  map;        // mmap address
  size_t map_size;   // Total mapped size
  void*  header;     // Header pointer
  void*  buckets;    // Buckets/slots start (after header)
  void*  pool;       // Entry pool start (for linked list indexes)
  size_t pool_size;  // Entry pool size
} NostrDBIndexContext;

// ============================================================================
// Callback types
// ============================================================================

/**
 * Callback for iterating over index entries
 * @param event_offset Offset of event in events.dat
 * @param created_at Event timestamp
 * @param user_data User-provided data
 * @return true to continue, false to stop
 */
typedef bool (*NostrDBIndexIterateCallback)(
  uint64_t event_offset,
  int64_t  created_at,
  void*    user_data);

#endif
