#include "nostr_response.h"

#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"

// ============================================================================
// Helper: Copy string to buffer with bounds check
// Returns number of characters that WOULD be written (for overflow detection)
// ============================================================================
static size_t safe_copy(char* dest, size_t dest_capacity, size_t offset, const char* src)
{
  if (src == NULL) {
    return 0;
  }

  size_t src_len = 0;
  while (src[src_len] != '\0') {
    src_len++;
  }

  if (dest == NULL || offset >= dest_capacity) {
    return src_len;  // Return how many chars would be needed
  }

  size_t i = 0;
  while (src[i] != '\0' && (offset + i) < dest_capacity - 1) {
    dest[offset + i] = src[i];
    i++;
  }
  return src_len;  // Return full source length for overflow detection
}

// ============================================================================
// Helper: Copy string with JSON escaping
// Returns number of characters that WOULD be written (for overflow detection)
// ============================================================================
static size_t safe_copy_json_escaped(char* dest, size_t dest_capacity, size_t offset, const char* src)
{
  if (src == NULL) {
    return 0;
  }

  // First, calculate the total length needed
  size_t total_needed = 0;
  for (size_t i = 0; src[i] != '\0'; i++) {
    char c = src[i];
    if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t') {
      total_needed += 2;  // Escaped chars take 2 chars
    } else {
      total_needed += 1;
    }
  }

  if (dest == NULL || offset >= dest_capacity) {
    return total_needed;
  }

  // Now actually write what we can
  size_t written = 0;
  size_t i       = 0;

  while (src[i] != '\0' && (offset + written) < dest_capacity - 1) {
    char c = src[i];

    if (c == '"' || c == '\\') {
      if ((offset + written + 1) >= dest_capacity - 1) {
        break;
      }
      dest[offset + written] = '\\';
      written++;
      dest[offset + written] = c;
      written++;
    } else if (c == '\n') {
      if ((offset + written + 1) >= dest_capacity - 1) {
        break;
      }
      dest[offset + written] = '\\';
      written++;
      dest[offset + written] = 'n';
      written++;
    } else if (c == '\r') {
      if ((offset + written + 1) >= dest_capacity - 1) {
        break;
      }
      dest[offset + written] = '\\';
      written++;
      dest[offset + written] = 'r';
      written++;
    } else if (c == '\t') {
      if ((offset + written + 1) >= dest_capacity - 1) {
        break;
      }
      dest[offset + written] = '\\';
      written++;
      dest[offset + written] = 't';
      written++;
    } else {
      dest[offset + written] = c;
      written++;
    }
    i++;
  }

  return total_needed;  // Return what WOULD be needed
}

// ============================================================================
// Helper: Write uint32 to buffer
// Returns number of characters written
// ============================================================================
static size_t write_uint32(char* dest, size_t dest_capacity, size_t offset, uint32_t value)
{
  if (dest == NULL || offset >= dest_capacity) {
    return 0;
  }

  // Convert to string (max 10 digits for uint32)
  char   temp[16];
  size_t temp_len = 0;

  if (value == 0) {
    temp[0]  = '0';
    temp_len = 1;
  } else {
    // Build digits in reverse
    while (value > 0 && temp_len < 15) {
      temp[temp_len++] = '0' + (value % 10);
      value /= 10;
    }
    // Reverse the string
    for (size_t i = 0; i < temp_len / 2; i++) {
      char t                 = temp[i];
      temp[i]                = temp[temp_len - 1 - i];
      temp[temp_len - 1 - i] = t;
    }
  }
  temp[temp_len] = '\0';

  return safe_copy(dest, dest_capacity, offset, temp);
}

// ============================================================================
// Helper: Write int64 (timestamp) to buffer
// Returns number of characters written
// ============================================================================
static size_t write_int64(char* dest, size_t dest_capacity, size_t offset, int64_t value)
{
  if (dest == NULL || offset >= dest_capacity) {
    return 0;
  }

  char   temp[24];
  size_t temp_len    = 0;
  bool   is_negative = (value < 0);

  if (is_negative) {
    value = -value;
  }

  if (value == 0) {
    temp[0]  = '0';
    temp_len = 1;
  } else {
    while (value > 0 && temp_len < 22) {
      temp[temp_len++] = '0' + (value % 10);
      value /= 10;
    }
    // Reverse
    for (size_t i = 0; i < temp_len / 2; i++) {
      char t                 = temp[i];
      temp[i]                = temp[temp_len - 1 - i];
      temp[temp_len - 1 - i] = t;
    }
  }

  if (is_negative) {
    // Shift and add minus sign
    for (size_t i = temp_len; i > 0; i--) {
      temp[i] = temp[i - 1];
    }
    temp[0] = '-';
    temp_len++;
  }

  temp[temp_len] = '\0';
  return safe_copy(dest, dest_capacity, offset, temp);
}

// ============================================================================
// Generate EOSE response: ["EOSE", "<subscription_id>"]
// ============================================================================
bool nostr_response_eose(
  const char* subscription_id,
  char*       buffer,
  size_t      capacity)
{
  require_not_null(subscription_id, false);
  require_not_null(buffer, false);
  require(capacity > 0, false);

  size_t pos = 0;

  pos += safe_copy(buffer, capacity, pos, "[\"EOSE\",\"");
  pos += safe_copy_json_escaped(buffer, capacity, pos, subscription_id);
  pos += safe_copy(buffer, capacity, pos, "\"]");

  // Check if we would overflow (need pos + 1 for null terminator)
  if (pos < capacity) {
    buffer[pos < capacity ? pos : capacity - 1] = '\0';
    return true;
  } else {
    buffer[capacity - 1] = '\0';
    return false;
  }
}

// ============================================================================
// Generate OK response: ["OK", "<event_id>", <success>, "<message>"]
// ============================================================================
bool nostr_response_ok(
  const char* event_id,
  bool        success,
  const char* message,
  char*       buffer,
  size_t      capacity)
{
  require_not_null(event_id, false);
  require_not_null(buffer, false);
  require(capacity > 0, false);

  size_t pos = 0;

  pos += safe_copy(buffer, capacity, pos, "[\"OK\",\"");
  pos += safe_copy_json_escaped(buffer, capacity, pos, event_id);
  pos += safe_copy(buffer, capacity, pos, "\",");
  pos += safe_copy(buffer, capacity, pos, success ? "true" : "false");
  pos += safe_copy(buffer, capacity, pos, ",\"");
  if (message != NULL) {
    pos += safe_copy_json_escaped(buffer, capacity, pos, message);
  }
  pos += safe_copy(buffer, capacity, pos, "\"]");

  if (pos < capacity) {
    buffer[pos < capacity ? pos : capacity - 1] = '\0';
    return true;
  } else {
    buffer[capacity - 1] = '\0';
    return false;
  }
}

// ============================================================================
// Generate NOTICE response: ["NOTICE", "<message>"]
// ============================================================================
bool nostr_response_notice(
  const char* message,
  char*       buffer,
  size_t      capacity)
{
  require_not_null(message, false);
  require_not_null(buffer, false);
  require(capacity > 0, false);

  size_t pos = 0;

  pos += safe_copy(buffer, capacity, pos, "[\"NOTICE\",\"");
  pos += safe_copy_json_escaped(buffer, capacity, pos, message);
  pos += safe_copy(buffer, capacity, pos, "\"]");

  if (pos < capacity) {
    buffer[pos < capacity ? pos : capacity - 1] = '\0';
    return true;
  } else {
    buffer[capacity - 1] = '\0';
    return false;
  }
}

// ============================================================================
// Generate CLOSED response: ["CLOSED", "<subscription_id>", "<message>"]
// ============================================================================
bool nostr_response_closed(
  const char* subscription_id,
  const char* message,
  char*       buffer,
  size_t      capacity)
{
  require_not_null(subscription_id, false);
  require_not_null(buffer, false);
  require(capacity > 0, false);

  size_t pos = 0;

  pos += safe_copy(buffer, capacity, pos, "[\"CLOSED\",\"");
  pos += safe_copy_json_escaped(buffer, capacity, pos, subscription_id);
  pos += safe_copy(buffer, capacity, pos, "\",\"");
  if (message != NULL) {
    pos += safe_copy_json_escaped(buffer, capacity, pos, message);
  }
  pos += safe_copy(buffer, capacity, pos, "\"]");

  if (pos < capacity) {
    buffer[pos < capacity ? pos : capacity - 1] = '\0';
    return true;
  } else {
    buffer[capacity - 1] = '\0';
    return false;
  }
}

// ============================================================================
// Helper: Serialize tags to JSON
// Returns number of characters written
// ============================================================================
static size_t serialize_tags(
  char*                 dest,
  size_t                dest_capacity,
  size_t                offset,
  const NostrTagEntity* tags,
  uint32_t              tag_count)
{
  size_t pos = offset;

  pos += safe_copy(dest, dest_capacity, pos, "[");

  for (uint32_t i = 0; i < tag_count; i++) {
    if (i > 0) {
      pos += safe_copy(dest, dest_capacity, pos, ",");
    }

    pos += safe_copy(dest, dest_capacity, pos, "[\"");
    pos += safe_copy_json_escaped(dest, dest_capacity, pos, tags[i].key);
    pos += safe_copy(dest, dest_capacity, pos, "\"");

    for (size_t j = 0; j < tags[i].item_count; j++) {
      pos += safe_copy(dest, dest_capacity, pos, ",\"");
      pos += safe_copy_json_escaped(dest, dest_capacity, pos, tags[i].values[j]);
      pos += safe_copy(dest, dest_capacity, pos, "\"");
    }

    pos += safe_copy(dest, dest_capacity, pos, "]");
  }

  pos += safe_copy(dest, dest_capacity, pos, "]");

  return pos - offset;
}

// ============================================================================
// Generate EVENT response: ["EVENT", "<subscription_id>", <event>]
// ============================================================================
bool nostr_response_event(
  const char*             subscription_id,
  const NostrEventEntity* event,
  char*                   buffer,
  size_t                  capacity)
{
  require_not_null(subscription_id, false);
  require_not_null(event, false);
  require_not_null(buffer, false);
  require(capacity > 0, false);

  size_t pos = 0;

  // Start array and EVENT type
  pos += safe_copy(buffer, capacity, pos, "[\"EVENT\",\"");
  pos += safe_copy_json_escaped(buffer, capacity, pos, subscription_id);
  pos += safe_copy(buffer, capacity, pos, "\",{");

  // id field
  pos += safe_copy(buffer, capacity, pos, "\"id\":\"");
  pos += safe_copy(buffer, capacity, pos, event->id);
  pos += safe_copy(buffer, capacity, pos, "\",");

  // pubkey field
  pos += safe_copy(buffer, capacity, pos, "\"pubkey\":\"");
  pos += safe_copy(buffer, capacity, pos, event->pubkey);
  pos += safe_copy(buffer, capacity, pos, "\",");

  // created_at field
  pos += safe_copy(buffer, capacity, pos, "\"created_at\":");
  pos += write_int64(buffer, capacity, pos, (int64_t)event->created_at);
  pos += safe_copy(buffer, capacity, pos, ",");

  // kind field
  pos += safe_copy(buffer, capacity, pos, "\"kind\":");
  pos += write_uint32(buffer, capacity, pos, event->kind);
  pos += safe_copy(buffer, capacity, pos, ",");

  // tags field
  pos += safe_copy(buffer, capacity, pos, "\"tags\":");
  pos += serialize_tags(buffer, capacity, pos, event->tags, event->tag_count);
  pos += safe_copy(buffer, capacity, pos, ",");

  // content field
  pos += safe_copy(buffer, capacity, pos, "\"content\":\"");
  pos += safe_copy_json_escaped(buffer, capacity, pos, event->content);
  pos += safe_copy(buffer, capacity, pos, "\",");

  // sig field
  pos += safe_copy(buffer, capacity, pos, "\"sig\":\"");
  pos += safe_copy(buffer, capacity, pos, event->sig);
  pos += safe_copy(buffer, capacity, pos, "\"");

  // Close event object and array
  pos += safe_copy(buffer, capacity, pos, "}]");

  if (pos < capacity) {
    buffer[pos] = '\0';
  } else {
    buffer[capacity - 1] = '\0';
    return false;
  }

  return true;
}
