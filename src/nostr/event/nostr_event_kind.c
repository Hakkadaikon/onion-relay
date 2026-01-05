#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_kind(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  uint32_t*        kind)
{
  if (!funcs->is_primitive(token)) {
    log_debug("Nostr Event Error: kind is not number\n");
    return false;
  }

  // TODO extract kind
  return true;
}
