#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

extern "C" {

// JSON types
typedef enum {
  JSMN_UNDEFINED = 0,
  JSMN_OBJECT    = 1 << 0,
  JSMN_ARRAY     = 1 << 1,
  JSMN_STRING    = 1 << 2,
  JSMN_PRIMITIVE = 1 << 3
} jsmntype_t;

typedef struct jsmntok {
  jsmntype_t type;
  int        start;
  int        end;
  int        size;
} jsmntok_t;

typedef struct jsmn_parser {
  unsigned int pos;
  unsigned int toknext;
  int          toksuper;
} jsmn_parser;

// Filter types
#define NOSTR_FILTER_MAX_IDS 256
#define NOSTR_FILTER_MAX_AUTHORS 256
#define NOSTR_FILTER_MAX_KINDS 64
#define NOSTR_FILTER_MAX_TAGS 26
#define NOSTR_FILTER_MAX_TAG_VALUES 256
#define NOSTR_FILTER_ID_LENGTH 32
#define NOSTR_FILTER_PUBKEY_LENGTH 32

typedef struct {
  uint8_t value[NOSTR_FILTER_ID_LENGTH];
  size_t  prefix_len;
} NostrFilterId;

typedef struct {
  uint8_t value[NOSTR_FILTER_PUBKEY_LENGTH];
  size_t  prefix_len;
} NostrFilterPubkey;

typedef struct {
  char    name;
  uint8_t values[NOSTR_FILTER_MAX_TAG_VALUES][32];
  size_t  values_count;
} NostrFilterTag;

typedef struct {
  NostrFilterId ids[NOSTR_FILTER_MAX_IDS];
  size_t        ids_count;
  NostrFilterPubkey authors[NOSTR_FILTER_MAX_AUTHORS];
  size_t            authors_count;
  uint32_t kinds[NOSTR_FILTER_MAX_KINDS];
  size_t   kinds_count;
  NostrFilterTag tags[NOSTR_FILTER_MAX_TAGS];
  size_t         tags_count;
  int64_t since;
  int64_t until;
  uint32_t limit;
} NostrFilter;

// REQ types
#define NOSTR_REQ_SUBSCRIPTION_ID_LENGTH 64
#define NOSTR_REQ_MAX_FILTERS 16

typedef struct {
  char        subscription_id[NOSTR_REQ_SUBSCRIPTION_ID_LENGTH + 1];
  NostrFilter filters[NOSTR_REQ_MAX_FILTERS];
  size_t      filters_count;
} NostrReqMessage;

// CLOSE types
#define NOSTR_CLOSE_SUBSCRIPTION_ID_LENGTH 64

typedef struct {
  char subscription_id[NOSTR_CLOSE_SUBSCRIPTION_ID_LENGTH + 1];
} NostrCloseMessage;

// Event types
#define NOSTR_EVENT_TAG_LENGTH (2 * 1024)
#define NOSTR_EVENT_TAG_VALUE_COUNT 16
#define NOSTR_EVENT_TAG_VALUE_LENGTH 512
#define NOSTR_EVENT_CONTENT_LENGTH (1 * 1024 * 1024)

typedef struct {
  char   key[64];
  char   values[NOSTR_EVENT_TAG_VALUE_COUNT][NOSTR_EVENT_TAG_VALUE_LENGTH];
  size_t item_count;
} NostrTagEntity;

typedef struct {
  char           id[65];
  char           dummy1[7];
  char           pubkey[65];
  char           dummy2[7];
  uint32_t       kind;
  uint32_t       tag_count;
  uint64_t       created_at;
  NostrTagEntity tags[NOSTR_EVENT_TAG_LENGTH];
  char           content[NOSTR_EVENT_CONTENT_LENGTH];
  char           sig[129];
  char           dummy3[7];
} NostrEventEntity;

// Subscription types
#define NOSTR_SUBSCRIPTION_MAX_COUNT 256

typedef struct {
  bool         active;
  int32_t      client_fd;
  char         subscription_id[NOSTR_REQ_SUBSCRIPTION_ID_LENGTH + 1];
  NostrFilter  filters[NOSTR_REQ_MAX_FILTERS];
  size_t       filters_count;
} NostrSubscription;

typedef struct {
  NostrSubscription* subscriptions;
  size_t             count;
} NostrSubscriptionManager;

// JSON function pointers
typedef bool (*PJsonStrCmpCallback)(const char* json, const jsmntok_t* token, const char* str, const size_t str_len);
typedef int32_t (*PJsonParseCallback)(jsmn_parser* parser, const char* json, const size_t json_len, jsmntok_t* tokens, const uint32_t num_tokens);
typedef bool (*PJsonInitCallback)(jsmn_parser* parser);
typedef bool (*PJsonTokenIsArrayCallback)(const jsmntok_t* token);
typedef bool (*PJsonTokenIsObjectCallback)(const jsmntok_t* token);
typedef bool (*PJsonTokenIsStringCallback)(const jsmntok_t* token);
typedef bool (*PJsonTokenIsPrimitiveCallback)(const jsmntok_t* token);
typedef size_t (*PGetJsonTokenLengthCallback)(const jsmntok_t* token);

typedef struct {
  PJsonInitCallback             init;
  PJsonStrCmpCallback           strncmp;
  PJsonParseCallback            parse;
  PJsonTokenIsObjectCallback    is_object;
  PJsonTokenIsArrayCallback     is_array;
  PJsonTokenIsStringCallback    is_string;
  PJsonTokenIsPrimitiveCallback is_primitive;
  PGetJsonTokenLengthCallback   get_token_length;
} JsonFuncs;

// Function declarations
bool json_funcs_init(JsonFuncs* funcs);
void jsmn_init(jsmn_parser* parser);
int jsmn_parse(jsmn_parser* parser, const char* json, const size_t json_len, jsmntok_t* tokens, const unsigned int num_tokens);

// Filter functions
void nostr_filter_init(NostrFilter* filter);
bool nostr_filter_parse(const JsonFuncs* funcs, const char* json, const jsmntok_t* token, const size_t token_count, NostrFilter* filter);
bool nostr_filter_matches(const NostrFilter* filter, const NostrEventEntity* event);
void nostr_filter_clear(NostrFilter* filter);

// REQ functions
void nostr_req_init(NostrReqMessage* req);
bool nostr_req_parse(const JsonFuncs* funcs, const char* json, const jsmntok_t* tokens, const size_t token_count, NostrReqMessage* req);
void nostr_req_clear(NostrReqMessage* req);

// CLOSE functions
void nostr_close_init(NostrCloseMessage* close_msg);
bool nostr_close_parse(const JsonFuncs* funcs, const char* json, const jsmntok_t* tokens, const size_t token_count, NostrCloseMessage* close_msg);
void nostr_close_clear(NostrCloseMessage* close_msg);

// Subscription functions
bool nostr_subscription_manager_init(NostrSubscriptionManager* manager);
void nostr_subscription_manager_destroy(NostrSubscriptionManager* manager);
NostrSubscription* nostr_subscription_add(NostrSubscriptionManager* manager, int32_t client_fd, const NostrReqMessage* req);
bool nostr_subscription_remove(NostrSubscriptionManager* manager, int32_t client_fd, const char* subscription_id);
size_t nostr_subscription_remove_client(NostrSubscriptionManager* manager, int32_t client_fd);
NostrSubscription* nostr_subscription_find(NostrSubscriptionManager* manager, int32_t client_fd, const char* subscription_id);
bool nostr_subscription_matches_event(const NostrSubscription* subscription, const NostrEventEntity* event);

}  // extern "C"

class NostrSubscriptionTest : public ::testing::Test {
protected:
  void SetUp() override {
    json_funcs_init(&funcs);
    memset(&filter, 0, sizeof(filter));
    memset(&req, 0, sizeof(req));
    memset(&close_msg, 0, sizeof(close_msg));
    memset(&event, 0, sizeof(event));
    memset(&manager, 0, sizeof(manager));
  }

  void TearDown() override {
    nostr_subscription_manager_destroy(&manager);
    memset(&manager, 0, sizeof(manager));
  }

  int parseJson(const char* json) {
    jsmn_parser parser;
    jsmn_init(&parser);
    return jsmn_parse(&parser, json, strlen(json), tokens, 256);
  }

  JsonFuncs funcs;
  NostrFilter filter;
  NostrReqMessage req;
  NostrCloseMessage close_msg;
  NostrEventEntity event;
  NostrSubscriptionManager manager;
  jsmntok_t tokens[256];
};

// ============================================================================
// Filter Init Tests
// ============================================================================
TEST_F(NostrSubscriptionTest, FilterInit_ZerosAllFields) {
  filter.ids_count = 10;
  filter.authors_count = 5;
  filter.since = 12345;

  nostr_filter_init(&filter);

  EXPECT_EQ(filter.ids_count, 0u);
  EXPECT_EQ(filter.authors_count, 0u);
  EXPECT_EQ(filter.kinds_count, 0u);
  EXPECT_EQ(filter.tags_count, 0u);
  EXPECT_EQ(filter.since, 0);
  EXPECT_EQ(filter.until, 0);
  EXPECT_EQ(filter.limit, 0u);
}

// ============================================================================
// Filter Parse Tests
// ============================================================================
TEST_F(NostrSubscriptionTest, FilterParse_EmptyObject) {
  const char* json = "{}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.ids_count, 0u);
  EXPECT_EQ(filter.kinds_count, 0u);
}

TEST_F(NostrSubscriptionTest, FilterParse_KindsArray) {
  const char* json = "{\"kinds\":[1,4,7]}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.kinds_count, 3u);
  EXPECT_EQ(filter.kinds[0], 1u);
  EXPECT_EQ(filter.kinds[1], 4u);
  EXPECT_EQ(filter.kinds[2], 7u);
}

TEST_F(NostrSubscriptionTest, FilterParse_Since) {
  const char* json = "{\"since\":1700000000}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.since, 1700000000);
}

TEST_F(NostrSubscriptionTest, FilterParse_Until) {
  const char* json = "{\"until\":1800000000}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.until, 1800000000);
}

TEST_F(NostrSubscriptionTest, FilterParse_Limit) {
  const char* json = "{\"limit\":100}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.limit, 100u);
}

TEST_F(NostrSubscriptionTest, FilterParse_Ids) {
  const char* json = "{\"ids\":[\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"]}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.ids_count, 1u);
  EXPECT_EQ(filter.ids[0].prefix_len, 32u);
  EXPECT_EQ(filter.ids[0].value[0], 0xaa);
  EXPECT_EQ(filter.ids[0].value[31], 0xaa);
}

TEST_F(NostrSubscriptionTest, FilterParse_IdsPrefix) {
  const char* json = "{\"ids\":[\"aabb\"]}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.ids_count, 1u);
  EXPECT_EQ(filter.ids[0].prefix_len, 2u);
  EXPECT_EQ(filter.ids[0].value[0], 0xaa);
  EXPECT_EQ(filter.ids[0].value[1], 0xbb);
}

TEST_F(NostrSubscriptionTest, FilterParse_Authors) {
  const char* json = "{\"authors\":[\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"]}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.authors_count, 1u);
  EXPECT_EQ(filter.authors[0].prefix_len, 32u);
  EXPECT_EQ(filter.authors[0].value[0], 0xbb);
}

TEST_F(NostrSubscriptionTest, FilterParse_ComplexFilter) {
  const char* json = "{\"kinds\":[1],\"since\":1700000000,\"until\":1800000000,\"limit\":50}";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_filter_parse(&funcs, json, tokens, count, &filter);
  EXPECT_TRUE(result);
  EXPECT_EQ(filter.kinds_count, 1u);
  EXPECT_EQ(filter.kinds[0], 1u);
  EXPECT_EQ(filter.since, 1700000000);
  EXPECT_EQ(filter.until, 1800000000);
  EXPECT_EQ(filter.limit, 50u);
}

// ============================================================================
// Filter Match Tests
// ============================================================================
TEST_F(NostrSubscriptionTest, FilterMatches_EmptyFilter_MatchesAll) {
  nostr_filter_init(&filter);
  strcpy(event.id, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  strcpy(event.pubkey, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  event.kind = 1;
  event.created_at = 1700000000;

  bool result = nostr_filter_matches(&filter, &event);
  EXPECT_TRUE(result);
}

TEST_F(NostrSubscriptionTest, FilterMatches_KindFilter_Match) {
  nostr_filter_init(&filter);
  filter.kinds[0] = 1;
  filter.kinds_count = 1;

  strcpy(event.id, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  strcpy(event.pubkey, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  event.kind = 1;
  event.created_at = 1700000000;

  bool result = nostr_filter_matches(&filter, &event);
  EXPECT_TRUE(result);
}

TEST_F(NostrSubscriptionTest, FilterMatches_KindFilter_NoMatch) {
  nostr_filter_init(&filter);
  filter.kinds[0] = 4;
  filter.kinds_count = 1;

  event.kind = 1;

  bool result = nostr_filter_matches(&filter, &event);
  EXPECT_FALSE(result);
}

TEST_F(NostrSubscriptionTest, FilterMatches_SinceFilter_Match) {
  nostr_filter_init(&filter);
  filter.since = 1700000000;

  strcpy(event.id, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  strcpy(event.pubkey, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  event.created_at = 1700000001;

  bool result = nostr_filter_matches(&filter, &event);
  EXPECT_TRUE(result);
}

TEST_F(NostrSubscriptionTest, FilterMatches_SinceFilter_NoMatch) {
  nostr_filter_init(&filter);
  filter.since = 1700000000;

  event.created_at = 1699999999;

  bool result = nostr_filter_matches(&filter, &event);
  EXPECT_FALSE(result);
}

TEST_F(NostrSubscriptionTest, FilterMatches_UntilFilter_Match) {
  nostr_filter_init(&filter);
  filter.until = 1700000000;

  strcpy(event.id, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  strcpy(event.pubkey, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  event.created_at = 1699999999;

  bool result = nostr_filter_matches(&filter, &event);
  EXPECT_TRUE(result);
}

TEST_F(NostrSubscriptionTest, FilterMatches_UntilFilter_NoMatch) {
  nostr_filter_init(&filter);
  filter.until = 1700000000;

  event.created_at = 1700000001;

  bool result = nostr_filter_matches(&filter, &event);
  EXPECT_FALSE(result);
}

// ============================================================================
// REQ Parse Tests
// ============================================================================
TEST_F(NostrSubscriptionTest, ReqParse_SimpleReq) {
  const char* json = "[\"REQ\",\"sub1\",{\"kinds\":[1]}]";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_req_parse(&funcs, json, tokens, count, &req);
  EXPECT_TRUE(result);
  EXPECT_STREQ(req.subscription_id, "sub1");
  EXPECT_EQ(req.filters_count, 1u);
  EXPECT_EQ(req.filters[0].kinds_count, 1u);
  EXPECT_EQ(req.filters[0].kinds[0], 1u);
}

TEST_F(NostrSubscriptionTest, ReqParse_MultipleFilters) {
  const char* json = "[\"REQ\",\"test-sub\",{\"kinds\":[1]},{\"kinds\":[4]}]";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_req_parse(&funcs, json, tokens, count, &req);
  EXPECT_TRUE(result);
  EXPECT_STREQ(req.subscription_id, "test-sub");
  EXPECT_EQ(req.filters_count, 2u);
  EXPECT_EQ(req.filters[0].kinds[0], 1u);
  EXPECT_EQ(req.filters[1].kinds[0], 4u);
}

TEST_F(NostrSubscriptionTest, ReqParse_InvalidNotReq) {
  const char* json = "[\"EVENT\",{\"id\":\"aaa\"}]";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_req_parse(&funcs, json, tokens, count, &req);
  EXPECT_FALSE(result);
}

TEST_F(NostrSubscriptionTest, ReqParse_TooFewElements) {
  const char* json = "[\"REQ\",\"sub1\"]";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_req_parse(&funcs, json, tokens, count, &req);
  EXPECT_FALSE(result);
}

// ============================================================================
// CLOSE Parse Tests
// ============================================================================
TEST_F(NostrSubscriptionTest, CloseParse_Valid) {
  const char* json = "[\"CLOSE\",\"sub1\"]";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_close_parse(&funcs, json, tokens, count, &close_msg);
  EXPECT_TRUE(result);
  EXPECT_STREQ(close_msg.subscription_id, "sub1");
}

TEST_F(NostrSubscriptionTest, CloseParse_InvalidNotClose) {
  const char* json = "[\"REQ\",\"sub1\"]";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_close_parse(&funcs, json, tokens, count, &close_msg);
  EXPECT_FALSE(result);
}

TEST_F(NostrSubscriptionTest, CloseParse_TooFewElements) {
  const char* json = "[\"CLOSE\"]";
  int count = parseJson(json);
  ASSERT_GT(count, 0);

  bool result = nostr_close_parse(&funcs, json, tokens, count, &close_msg);
  EXPECT_FALSE(result);
}

// ============================================================================
// Subscription Manager Tests
// ============================================================================
TEST_F(NostrSubscriptionTest, SubscriptionManager_Init) {
  nostr_subscription_manager_init(&manager);
  EXPECT_EQ(manager.count, 0u);
  EXPECT_FALSE(manager.subscriptions[0].active);
}

TEST_F(NostrSubscriptionTest, SubscriptionManager_AddAndFind) {
  nostr_subscription_manager_init(&manager);

  // Parse a REQ to add
  const char* json = "[\"REQ\",\"test-sub\",{\"kinds\":[1]}]";
  int count = parseJson(json);
  nostr_req_parse(&funcs, json, tokens, count, &req);

  NostrSubscription* sub = nostr_subscription_add(&manager, 42, &req);
  ASSERT_NE(sub, nullptr);
  EXPECT_TRUE(sub->active);
  EXPECT_EQ(sub->client_fd, 42);
  EXPECT_STREQ(sub->subscription_id, "test-sub");
  EXPECT_EQ(manager.count, 1u);

  // Find it
  NostrSubscription* found = nostr_subscription_find(&manager, 42, "test-sub");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, sub);
}

TEST_F(NostrSubscriptionTest, SubscriptionManager_Remove) {
  nostr_subscription_manager_init(&manager);

  const char* json = "[\"REQ\",\"test-sub\",{\"kinds\":[1]}]";
  int count = parseJson(json);
  nostr_req_parse(&funcs, json, tokens, count, &req);

  nostr_subscription_add(&manager, 42, &req);
  EXPECT_EQ(manager.count, 1u);

  bool removed = nostr_subscription_remove(&manager, 42, "test-sub");
  EXPECT_TRUE(removed);
  EXPECT_EQ(manager.count, 0u);

  NostrSubscription* found = nostr_subscription_find(&manager, 42, "test-sub");
  EXPECT_EQ(found, nullptr);
}

TEST_F(NostrSubscriptionTest, SubscriptionManager_RemoveClient) {
  nostr_subscription_manager_init(&manager);

  // Add two subscriptions for the same client
  const char* json1 = "[\"REQ\",\"sub1\",{\"kinds\":[1]}]";
  int count1 = parseJson(json1);
  nostr_req_parse(&funcs, json1, tokens, count1, &req);
  nostr_subscription_add(&manager, 42, &req);

  const char* json2 = "[\"REQ\",\"sub2\",{\"kinds\":[4]}]";
  int count2 = parseJson(json2);
  nostr_req_parse(&funcs, json2, tokens, count2, &req);
  nostr_subscription_add(&manager, 42, &req);

  EXPECT_EQ(manager.count, 2u);

  size_t removed = nostr_subscription_remove_client(&manager, 42);
  EXPECT_EQ(removed, 2u);
  EXPECT_EQ(manager.count, 0u);
}

TEST_F(NostrSubscriptionTest, SubscriptionManager_UpdateExisting) {
  nostr_subscription_manager_init(&manager);

  // Add subscription
  const char* json1 = "[\"REQ\",\"test-sub\",{\"kinds\":[1]}]";
  int count1 = parseJson(json1);
  nostr_req_parse(&funcs, json1, tokens, count1, &req);
  NostrSubscription* sub1 = nostr_subscription_add(&manager, 42, &req);
  EXPECT_EQ(sub1->filters[0].kinds[0], 1u);

  // Update with same subscription_id
  const char* json2 = "[\"REQ\",\"test-sub\",{\"kinds\":[4]}]";
  int count2 = parseJson(json2);
  nostr_req_parse(&funcs, json2, tokens, count2, &req);
  NostrSubscription* sub2 = nostr_subscription_add(&manager, 42, &req);

  EXPECT_EQ(sub1, sub2);  // Same pointer
  EXPECT_EQ(manager.count, 1u);  // Still only 1
  EXPECT_EQ(sub2->filters[0].kinds[0], 4u);  // Updated
}

TEST_F(NostrSubscriptionTest, SubscriptionMatchesEvent_Match) {
  nostr_subscription_manager_init(&manager);

  const char* json = "[\"REQ\",\"test-sub\",{\"kinds\":[1]}]";
  int count = parseJson(json);
  nostr_req_parse(&funcs, json, tokens, count, &req);
  NostrSubscription* sub = nostr_subscription_add(&manager, 42, &req);

  strcpy(event.id, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  strcpy(event.pubkey, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  event.kind = 1;
  event.created_at = 1700000000;

  bool matches = nostr_subscription_matches_event(sub, &event);
  EXPECT_TRUE(matches);
}

TEST_F(NostrSubscriptionTest, SubscriptionMatchesEvent_NoMatch) {
  nostr_subscription_manager_init(&manager);

  const char* json = "[\"REQ\",\"test-sub\",{\"kinds\":[4]}]";
  int count = parseJson(json);
  nostr_req_parse(&funcs, json, tokens, count, &req);
  NostrSubscription* sub = nostr_subscription_add(&manager, 42, &req);

  event.kind = 1;  // Not kind 4

  bool matches = nostr_subscription_matches_event(sub, &event);
  EXPECT_FALSE(matches);
}
