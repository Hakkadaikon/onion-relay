#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_created_at(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  time_t*          created_at)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(created_at, false);

  if (!funcs->is_primitive(token)) {
    log_debug("Nostr Event Error: created_at is not number\n");
    return false;
  }

  // Parse integer from token
  size_t len = funcs->get_token_length(token);
  if (len == 0) {
    log_debug("Nostr Event Error: created_at length invalid\n");
    return false;
  }

  if (len > 24) {  // Allow room for sign, digits, 'e', '+', exponent
    log_debug("Nostr Event Error: created_at length invalid\n");
    return false;
  }

  size_t pos      = 0;
  bool   negative = false;

  // Handle negative sign
  char first = json[token->start + pos];
  if (first == '-') {
    negative = true;
    pos++;
  }

  // Parse mantissa digits
  time_t value = 0;
  while (pos < len && is_digit(json[token->start + pos])) {
    value = value * 10 + (time_t)(json[token->start + pos] - '0');
    pos++;
  }

  // Handle scientific notation (e.g., 1e+10)
  if (pos < len && (json[token->start + pos] == 'e' || json[token->start + pos] == 'E')) {
    pos++;  // skip 'e'/'E'

    bool exp_negative = false;
    if (pos < len && json[token->start + pos] == '+') {
      pos++;
    } else if (pos < len && json[token->start + pos] == '-') {
      exp_negative = true;
      pos++;
    }

    int32_t exponent = 0;
    while (pos < len && is_digit(json[token->start + pos])) {
      exponent = exponent * 10 + (int32_t)(json[token->start + pos] - '0');
      pos++;
    }

    // Apply exponent
    if (exp_negative) {
      for (int32_t i = 0; i < exponent; i++) {
        value /= 10;
      }
    } else {
      for (int32_t i = 0; i < exponent; i++) {
        value *= 10;
      }
    }
  }

  // Handle decimal point (truncate fractional part for integer timestamp)
  if (pos < len && json[token->start + pos] == '.') {
    pos++;  // skip '.'
    while (pos < len && is_digit(json[token->start + pos])) {
      pos++;  // skip fractional digits
    }
  }

  if (negative) {
    value = -value;
  }

  *created_at = value;
  return true;
}
