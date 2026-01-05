#include "nostr_func.h"

#include "../util/log.h"
#include "../util/string.h"
#include "nostr_types.h"

bool nostr_event_handler(const char* json, PNostrFuncs nostr_funcs)
{
  JsonFuncs  json_funcs;
  JsonParser parser;
  jsontok_t  token[JSON_TOKEN_CAPACITY];

  json_funcs_init(&json_funcs);

  json_funcs.init(&parser);

  size_t json_len = strlen(json);

  int32_t token_count = json_funcs.parse(
    &parser,
    json,
    json_len,
    token,
    JSON_TOKEN_CAPACITY);

  if (token_count < 5) {
    log_debug("JSON error: Parse error\n");
    var_debug("token_count:", token_count);
    return false;
  }

  if (!json_funcs.is_array(&token[0])) {
    log_debug("JSON error: json is not array\n");
    return false;
  }

  if (!json_funcs.is_string(&token[1])) {
    log_debug("JSON error: array[0] is not a string\n");
    return false;
  }

  if (!json_funcs.strncmp(json, &token[1], "EVENT", 5)) {
    if (!json_funcs.is_object(&token[2])) {
      log_debug("JSON error: Invalid EVENT format\n");
      return false;
    }

    NostrEvent event;

    if (!extract_nostr_event(
          &json_funcs,
          json,
          &token[3],
          token_count - 3,
          &event)) {
      return false;
    }

    return nostr_funcs->event(&event);
  }

  if (!json_funcs.strncmp(json, &token[1], "REQ", 3)) {
  }

  return false;
}
