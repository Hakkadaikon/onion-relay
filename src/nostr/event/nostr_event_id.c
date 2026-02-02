#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_id(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  char*            id)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(id, false);

  if (!funcs->is_string(token)) {
    log_debug("Nostr Event Error: id is not string\n");
    return false;
  }

  size_t id_len = funcs->get_token_length(token);
  if (id_len != 64) {
    log_debug("Nostr Event Error: id is not 64 bytes\n");
    return false;
  }

  if (!is_lower_hex_str(&json[token->start], id_len)) {
    log_debug("Nostr Event Error: id is not hex\n");
    return false;
  }

  // Copy id string
  internal_memcpy(id, &json[token->start], 64);
  id[64] = '\0';

  return true;
}
