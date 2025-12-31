#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool is_valid_nostr_event_kind(const PJsonFuncs funcs, const char* json, const jsontok_t* token)
{
  if (!funcs->is_primitive(token)) {
    log_debug("Nostr Event Error: kind is not number\n");
    return false;
  }

  return true;
}
