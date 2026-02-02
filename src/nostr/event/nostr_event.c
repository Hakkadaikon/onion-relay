#include "../../util/allocator.h"
#include "../../util/log.h"
#include "../nostr_func.h"

extern bool extract_nostr_event_id(const PJsonFuncs funcs, const char* json, const jsontok_t* token, char* id);
extern bool extract_nostr_event_pubkey(const PJsonFuncs funcs, const char* json, const jsontok_t* token, char* pubkey);
extern bool extract_nostr_event_kind(const PJsonFuncs funcs, const char* json, const jsontok_t* token, uint32_t* kind);
extern bool extract_nostr_event_created_at(const PJsonFuncs funcs, const char* json, const jsontok_t* token, time_t* created_at);
extern bool extract_nostr_event_sig(const PJsonFuncs funcs, const char* json, const jsontok_t* token, char* sig);
extern bool extract_nostr_event_tags(const PJsonFuncs funcs, const char* json, const jsontok_t* token, NostrTagEntity* tags, uint32_t* tag_count);
extern bool extract_nostr_event_content(const PJsonFuncs funcs, const char* json, const jsontok_t* token, char* content, size_t content_capacity);

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

  for (int i = 0; i < token_count; i += 2) {
    int key_index   = i;
    int value_index = i + 1;

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

  return found.id &&
         found.pubkey &&
         found.kind &&
         found.created_at &&
         found.sig &&
         found.tags &&
         found.content;
}
