#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_sig(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  char*            sig)
{
  if (!funcs->is_string(token)) {
    log_debug("Nostr Event Error: sig is not string\n");
    return false;
  }

  size_t sig_len = funcs->get_token_length(token);
  if (sig_len != 128) {
    log_debug("Nostr Event Error: pubkey is not 128 bytes\n");
    return false;
  }

  if (!is_lower_hex_str(&json[token->start], sig_len)) {
    log_debug("Nostr Event Error: sig is not hex\n");
    return false;
  }

  // TODO extract sig
  return true;
}
