#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool is_valid_nostr_event_created_at(const PJsonFuncs funcs, const char* json, const jsontok_t* token)
{
  if (!funcs->is_primitive(token)) {
    log_debug("Nostr Event Error: created_at is not number\n");
    return false;
  }

  return true;
}
