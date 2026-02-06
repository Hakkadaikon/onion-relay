#include "../../util/allocator.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

extern bool extract_nostr_event_id(const PJsonFuncs funcs, const char* json, const jsontok_t* token, char* id);
extern bool extract_nostr_event_pubkey(const PJsonFuncs funcs, const char* json, const jsontok_t* token, char* pubkey);
extern bool extract_nostr_event_kind(const PJsonFuncs funcs, const char* json, const jsontok_t* token, uint32_t* kind);
extern bool extract_nostr_event_created_at(const PJsonFuncs funcs, const char* json, const jsontok_t* token, time_t* created_at);
extern bool extract_nostr_event_sig(const PJsonFuncs funcs, const char* json, const jsontok_t* token, char* sig);
extern bool extract_nostr_event_tags(const PJsonFuncs funcs, const char* json, const jsontok_t* token, NostrTagEntity* tags, uint32_t* tag_count);
extern bool extract_nostr_event_content(const PJsonFuncs funcs, const char* json, const jsontok_t* token, char* content, size_t content_capacity);

// Count total tokens for a JSMN token subtree (including the token itself)
static int count_value_tokens(const jsontok_t* token, int remaining)
{
  if (remaining <= 0) {
    return 0;
  }

  if (token->type == JSMN_PRIMITIVE || token->type == JSMN_STRING) {
    return 1;
  }

  int count    = 1;  // The container token itself
  int children = token->size;

  if (token->type == JSMN_OBJECT) {
    // Object: each child is a key-value pair
    for (int i = 0; i < children && count < remaining; i++) {
      count++;  // key token
      count += count_value_tokens(&token[count], remaining - count);
    }
  } else if (token->type == JSMN_ARRAY) {
    // Array: each child is a value
    for (int i = 0; i < children && count < remaining; i++) {
      count += count_value_tokens(&token[count], remaining - count);
    }
  }

  return count;
}

bool extract_nostr_event(
  const PJsonFuncs  funcs,
  const char*       json,
  const jsontok_t*  token,
  const size_t      token_count,
  NostrEventEntity* event)
{
  struct {
    bool id;
    bool pubkey;
    bool kind;
    bool created_at;
    bool sig;
    bool tags;
    bool content;
  } found;

  websocket_memset(&found, 0x00, sizeof(found));

  for (int i = 0; i < token_count;) {
    int key_index   = i;
    int value_index = i + 1;

    if (value_index >= (int)token_count) {
      break;
    }

    // Compute token count for value subtree and advance i past key + value
    int value_tokens = count_value_tokens(&token[value_index], (int)token_count - value_index);
    i += 1 + value_tokens;

    if (!funcs->is_string(&token[key_index])) {
      log_debug("JSON error: key is not string\n");
      return false;
    }

    if (funcs->strncmp(json, &token[key_index], "id", 2)) {
      found.id = true;
      log_debug("id found\n");

      if (!extract_nostr_event_id(funcs, json, &token[value_index], event->id)) {
        return false;
      }

      continue;
    }

    if (funcs->strncmp(json, &token[key_index], "pubkey", 6)) {
      found.pubkey = true;
      log_debug("pubkey found\n");

      if (!extract_nostr_event_pubkey(funcs, json, &token[value_index], event->pubkey)) {
        return false;
      }

      continue;
    }

    if (funcs->strncmp(json, &token[key_index], "kind", 4)) {
      found.kind = true;
      log_debug("kind found\n");

      if (!extract_nostr_event_kind(funcs, json, &token[value_index], &event->kind)) {
        return false;
      }

      continue;
    }

    if (funcs->strncmp(json, &token[key_index], "created_at", 10)) {
      found.created_at = true;
      log_debug("created_at found\n");

      if (!extract_nostr_event_created_at(funcs, json, &token[value_index], &event->created_at)) {
        return false;
      }

      continue;
    }

    if (funcs->strncmp(json, &token[key_index], "sig", 3)) {
      found.sig = true;
      log_debug("sig found\n");

      if (!extract_nostr_event_sig(funcs, json, &token[value_index], event->sig)) {
        return false;
      }

      continue;
    }

    if (funcs->strncmp(json, &token[key_index], "tags", 4)) {
      found.tags = true;
      log_debug("tags found\n");

      if (!extract_nostr_event_tags(funcs, json, &token[value_index], event->tags, &event->tag_count)) {
        return false;
      }

      continue;
    }

    if (funcs->strncmp(json, &token[key_index], "content", 7)) {
      found.content = true;
      log_debug("content found\n");

      if (!extract_nostr_event_content(funcs, json, &token[value_index], event->content, sizeof(event->content))) {
        return false;
      }

      continue;
    }
  }

  require(found.id, false);
  require(found.pubkey, false);
  require(found.kind, false);
  require(found.created_at, false);
  require(found.sig, false);
  require(found.tags, false);
  require(found.content, false);

  return true;
}
