#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "nostr_filter.h"

// ============================================================================
// Helper: Convert hex char to value
// ============================================================================
static int32_t hex_char_value(char c)
{
  if (is_digit(c)) {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

// ============================================================================
// Helper: Convert hex string to bytes (supports partial/prefix)
// Returns number of bytes written
// ============================================================================
static size_t hex_to_bytes(const char* hex, size_t hex_len, uint8_t* out, size_t out_capacity)
{
  // Only process even number of hex chars
  size_t pairs = hex_len / 2;
  if (pairs > out_capacity) {
    pairs = out_capacity;
  }

  for (size_t i = 0; i < pairs; i++) {
    int32_t h = hex_char_value(hex[i * 2]);
    int32_t l = hex_char_value(hex[i * 2 + 1]);

    if (h < 0 || l < 0) {
      return i;  // Stop at invalid hex
    }

    out[i] = (uint8_t)((h << 4) | l);
  }

  return pairs;
}

bool extract_nostr_filter_authors(
  const PJsonFuncs   funcs,
  const char*        json,
  const jsontok_t*   token,
  NostrFilterPubkey* authors,
  size_t*            authors_count)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(authors, false);
  require_not_null(authors_count, false);

  if (!funcs->is_array(token)) {
    log_debug("Filter error: authors is not an array\n");
    return false;
  }

  int num_authors = token->size;
  if (num_authors < 0) {
    log_debug("Filter error: invalid authors array size\n");
    return false;
  }

  *authors_count = 0;

  if (num_authors == 0) {
    return true;
  }

  int token_idx = 1;

  for (int i = 0; i < num_authors && *authors_count < NOSTR_FILTER_MAX_AUTHORS; i++) {
    const jsontok_t* author_token = &token[token_idx];

    if (!funcs->is_string(author_token)) {
      log_debug("Filter error: author element is not a string\n");
      token_idx++;
      continue;
    }

    size_t hex_len = funcs->get_token_length(author_token);
    if (hex_len == 0 || hex_len > 64) {
      token_idx++;
      continue;
    }

    // Initialize the filter entry
    NostrFilterPubkey* entry = &authors[*authors_count];
    internal_memset(entry, 0, sizeof(NostrFilterPubkey));

    // Convert hex to bytes
    size_t bytes_written = hex_to_bytes(
      &json[author_token->start],
      hex_len,
      entry->value,
      NOSTR_FILTER_PUBKEY_LENGTH);

    if (bytes_written > 0) {
      entry->prefix_len = bytes_written;
      (*authors_count)++;
    }

    token_idx++;
  }

  return true;
}
