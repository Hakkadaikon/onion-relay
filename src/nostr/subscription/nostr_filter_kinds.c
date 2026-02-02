#include "../../util/log.h"
#include "../../util/string.h"
#include "nostr_filter.h"

bool extract_nostr_filter_kinds(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  uint32_t*        kinds,
  size_t*          kinds_count)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(kinds, false);
  require_not_null(kinds_count, false);

  if (!funcs->is_array(token)) {
    log_debug("Filter error: kinds is not an array\n");
    return false;
  }

  int num_kinds = token->size;
  if (num_kinds < 0) {
    log_debug("Filter error: invalid kinds array size\n");
    return false;
  }

  *kinds_count = 0;

  if (num_kinds == 0) {
    return true;
  }

  int token_idx = 1;  // Skip array token

  for (int i = 0; i < num_kinds && *kinds_count < NOSTR_FILTER_MAX_KINDS; i++) {
    const jsontok_t* kind_token = &token[token_idx];

    if (!funcs->is_primitive(kind_token)) {
      log_debug("Filter error: kind element is not a number\n");
      token_idx++;
      continue;
    }

    size_t len = funcs->get_token_length(kind_token);
    if (len == 0 || len > 10) {
      token_idx++;
      continue;
    }

    uint32_t value = 0;
    bool     valid = true;
    for (size_t j = 0; j < len; j++) {
      char c = json[kind_token->start + j];
      if (!is_digit(c)) {
        valid = false;
        break;
      }
      value = value * 10 + (uint32_t)(c - '0');
    }

    if (valid) {
      kinds[*kinds_count] = value;
      (*kinds_count)++;
    }

    token_idx++;
  }

  return true;
}
