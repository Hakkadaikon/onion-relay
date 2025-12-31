#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool is_valid_nostr_event_tags(const PJsonFuncs funcs, const char* json, const jsontok_t* token)
{
  if (!funcs->is_array(token)) {
    log_debug("JSON error: tags is not array\n");
    return false;
  }

  return true;
}
