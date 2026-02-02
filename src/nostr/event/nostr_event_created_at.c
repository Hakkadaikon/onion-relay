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
  if (len == 0 || len > 20) {  // Max 20 digits for int64/time_t
    log_debug("Nostr Event Error: created_at length invalid\n");
    return false;
  }

  time_t value = 0;
  for (size_t i = 0; i < len; i++) {
    char c = json[token->start + i];
    if (!is_digit(c)) {
      log_debug("Nostr Event Error: created_at contains non-digit\n");
      return false;
    }
    value = value * 10 + (time_t)(c - '0');
  }

  *created_at = value;
  return true;
}
