#include "../../util/log.h"
#include "../../util/string.h"
#include "nostr_filter.h"

bool extract_nostr_filter_limit(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  uint32_t*        limit)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(limit, false);

  if (!funcs->is_primitive(token)) {
    log_debug("Filter error: limit is not a number\n");
    return false;
  }

  size_t len = funcs->get_token_length(token);
  if (len == 0 || len > 10) {
    log_debug("Filter error: limit length invalid\n");
    return false;
  }

  uint32_t value = 0;
  for (size_t i = 0; i < len; i++) {
    char c = json[token->start + i];
    if (!is_digit(c)) {
      log_debug("Filter error: limit contains non-digit\n");
      return false;
    }
    value = value * 10 + (uint32_t)(c - '0');
  }

  *limit = value;
  return true;
}
