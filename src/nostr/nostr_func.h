#ifndef NOSTR_FUNCS_H_
#define NOSTR_FUNCS_H_

#define JSMN_HEADER
#include "../json/json_wrapper.h"
#include "nostr_types.h"

typedef bool (*PNostrEventCallback)(const NostrEvent* event);
typedef bool (*PNostrRequestCallback)(const NostrRequest* req);

typedef struct {
  PNostrEventCallback   event;
  PNostrRequestCallback req;
} NostrFuncs, *PNostrFuncs;

bool extract_nostr_event(const PJsonFuncs funcs, const char* json, const jsontok_t* token, const size_t token_count, NostrEvent* event);
bool nostr_event_handler(const char* json, PNostrFuncs nostr_funcs);

#endif
