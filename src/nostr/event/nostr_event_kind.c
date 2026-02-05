#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_kind(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  uint32_t*        kind)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(kind, false);

  if (!funcs->is_primitive(token)) {
    log_debug("Nostr Event Error: kind is not number\n");
    return false;
  }

  // Parse integer from token
  size_t len = funcs->get_token_length(token);
  if (len == 0) {
    log_debug("Nostr Event Error: kind length invalid\n");
    return false;
  }

  if (len > 10) {  // Max 10 digits for uint32
    log_debug("Nostr Event Error: kind length invalid\n");
    return false;
  }

  uint32_t value = 0;
  for (size_t i = 0; i < len; i++) {
    char c = json[token->start + i];
    if (!is_digit(c)) {
      log_debug("Nostr Event Error: kind contains non-digit\n");
      return false;
    }
    value = value * 10 + (uint32_t)(c - '0');
  }

  *kind = value;
  return true;
}
