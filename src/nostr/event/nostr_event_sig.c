#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_sig(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  char*            sig)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(sig, false);

  if (!funcs->is_string(token)) {
    log_debug("Nostr Event Error: sig is not string\n");
    return false;
  }

  size_t sig_len = funcs->get_token_length(token);
  if (sig_len != 128) {
    log_debug("Nostr Event Error: sig is not 128 bytes\n");
    return false;
  }

  if (!is_lower_hex_str(&json[token->start], sig_len)) {
    log_debug("Nostr Event Error: sig is not hex\n");
    return false;
  }

  // Copy sig string
  internal_memcpy(sig, &json[token->start], 128);
  sig[128] = '\0';

  return true;
}
