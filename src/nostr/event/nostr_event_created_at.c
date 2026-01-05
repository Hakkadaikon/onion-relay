#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_created_at(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  time_t*          created_at)
{
  if (!funcs->is_primitive(token)) {
    log_debug("Nostr Event Error: created_at is not number\n");
    return false;
  }

  // TODO extract created_at
  return true;
}
