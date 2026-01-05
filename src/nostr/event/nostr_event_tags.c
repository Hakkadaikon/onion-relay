#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

bool extract_nostr_event_tags(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  NostrTagEntity*  tags)
{
  if (!funcs->is_array(token)) {
    log_debug("JSON error: tags is not array\n");
    return false;
  }

  // TODO extract tags
  return true;
}
