#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_id(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  char*            id)
{
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
  }

  // TODO extract id
  return true;
}
