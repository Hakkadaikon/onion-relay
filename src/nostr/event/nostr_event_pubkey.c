#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_pubkey(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  char*            pubkey)
{
  if (!funcs->is_string(token)) {
    log_debug("Nostr Event Error: pubkey is not string\n");
    return false;
  }

  size_t pubkey_len = funcs->get_token_length(token);
  if (pubkey_len != 64) {
    log_debug("Nostr Event Error: pubkey is not 64 bytes\n");
    return false;
  }

  if (!is_lower_hex_str(&json[token->start], pubkey_len)) {
    log_debug("Nostr Event Error: pubkey is not hex\n");
    return false;
  }

  // TODO extract pubkey
  return true;
}
