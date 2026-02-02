#include "../../util/log.h"
#include "../../util/string.h"
#include "nostr_filter.h"

bool extract_nostr_filter_since(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  int64_t*         since)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(since, false);

  if (!funcs->is_primitive(token)) {
    log_debug("Filter error: since is not a number\n");
    return false;
  }

  size_t len = funcs->get_token_length(token);
  if (len == 0 || len > 20) {
    log_debug("Filter error: since length invalid\n");
    return false;
  }

  int64_t value = 0;
  for (size_t i = 0; i < len; i++) {
    char c = json[token->start + i];
    if (!is_digit(c)) {
      log_debug("Filter error: since contains non-digit\n");
      return false;
    }
    value = value * 10 + (int64_t)(c - '0');
  }

  *since = value;
  return true;
}
