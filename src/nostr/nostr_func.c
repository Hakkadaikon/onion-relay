#include "nostr_func.h"

#include "../util/log.h"
#include "../util/string.h"
#include "nostr_types.h"

bool json_to_nostr_event(const char* json, PNostrEvent event)
{
  JsonFuncs  funcs;
  JsonParser parser;
  jsontok_t  token[JSON_TOKEN_CAPACITY];

  json_funcs_init(&funcs);

  funcs.init(&parser);

  size_t json_len = strlen(json);

  int32_t token_count = funcs.parse(
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

  if (!funcs.is_array(&token[0])) {
    log_debug("JSON error: json is not array\n");
    return false;
  }

  if (!funcs.is_string(&token[1])) {
    log_debug("JSON error: array[0] is not a string\n");
    return false;
  }

  if (!funcs.strncmp(json, &token[1], "EVENT", 5)) {
    log_debug("JSON error: array[0] is not a \"EVENT\"\n");
    return false;
  }

  if (!funcs.is_object(&token[2])) {
    log_debug("JSON error: Invalid EVENT format\n");
    return false;
  }

  for (int i = 3; i < token_count; i += 2) {
    int key_index   = i;
    int value_index = i + 1;

    if (!funcs.is_string(&token[key_index])) {
      log_debug("JSON error: key is not string\n");
      return false;
    }

    if (funcs.strncmp(json, &token[key_index], "id", 2)) {
      log_debug("id found\n");

      if (!is_valid_nostr_event_id(&funcs, json, &token[value_index])) {
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "pubkey", 6)) {
      log_debug("pubkey found\n");

      if (!is_valid_nostr_event_pubkey(&funcs, json, &token[value_index])) {
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "kind", 4)) {
      log_debug("kind found\n");

      if (!is_valid_nostr_event_kind(&funcs, json, &token[value_index])) {
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "created_at", 10)) {
      log_debug("created_at found\n");

      if (!is_valid_nostr_event_created_at(&funcs, json, &token[value_index])) {
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "sig", 3)) {
      log_debug("sig found\n");

      if (!is_valid_nostr_event_sig(&funcs, json, &token[value_index])) {
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "tags", 4)) {
      log_debug("tags found\n");

      if (!is_valid_nostr_event_tags(&funcs, json, &token[value_index])) {
        return false;
      }

      continue;
    }
  }

  return true;
}
