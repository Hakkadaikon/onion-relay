#include "nostr_func.h"

#include "../json/json_wrapper.h"
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
      if (!funcs.is_string(&token[value_index])) {
        log_debug("Nostr Event Error: id is not string\n");
        return false;
      }

      size_t id_len = funcs.get_token_length(&token[value_index]);
      if (id_len != 64) {
        log_debug("Nostr Event Error: id is not 64 bytes\n");
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "pubkey", 6)) {
      log_debug("pubkey found\n");

      if (!funcs.is_string(&token[value_index])) {
        log_debug("Nostr Event Error: pubkey is not string\n");
        return false;
      }

      size_t pubkey_len = funcs.get_token_length(&token[value_index]);
      if (pubkey_len != 64) {
        log_debug("Nostr Event Error: pubkey is not 64 bytes\n");
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "kind", 4)) {
      log_debug("kind found\n");

      if (!funcs.is_primitive(&token[value_index])) {
        log_debug("Nostr Event Error: kind is not number\n");
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "created_at", 10)) {
      log_debug("created_at found\n");

      if (!funcs.is_primitive(&token[value_index])) {
        log_debug("Nostr Event Error: created_at is not number\n");
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "sig", 3)) {
      log_debug("sig found\n");

      if (!funcs.is_string(&token[value_index])) {
        log_debug("Nostr Event Error: sig is not string\n");
        return false;
      }

      size_t sig_len = funcs.get_token_length(&token[value_index]);
      if (sig_len != 128) {
        log_debug("Nostr Event Error: pubkey is not 128 bytes\n");
        return false;
      }

      continue;
    }

    if (funcs.strncmp(json, &token[key_index], "tags", 4)) {
      log_debug("tags found\n");

      if (!funcs.is_array(&token[value_index])) {
        log_debug("JSON error: tags is not array\n");
        return false;
      }

      continue;
    }
  }

  return true;
}
