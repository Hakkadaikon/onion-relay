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

// Nostr types
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

bool extract_nostr_event_id(const JsonFuncs* funcs, const char* json, const jsmntok_t* token, char* id);
bool extract_nostr_event_pubkey(const JsonFuncs* funcs, const char* json, const jsmntok_t* token, char* pubkey);
bool extract_nostr_event_sig(const JsonFuncs* funcs, const char* json, const jsmntok_t* token, char* sig);
bool extract_nostr_event_content(const JsonFuncs* funcs, const char* json, const jsmntok_t* token, char* content, size_t content_capacity);
bool extract_nostr_event_tags(const JsonFuncs* funcs, const char* json, const jsmntok_t* token, NostrTagEntity* tags, uint32_t* tag_count);
bool extract_nostr_event_kind(const JsonFuncs* funcs, const char* json, const jsmntok_t* token, uint32_t* kind);
bool extract_nostr_event_created_at(const JsonFuncs* funcs, const char* json, const jsmntok_t* token, uint64_t* created_at);

}  // extern "C"

class NostrEventTest : public ::testing::Test {
protected:
  void SetUp() override {
    json_funcs_init(&funcs);
    memset(&event, 0, sizeof(event));
  }

  // Parse JSON and return token count
  int parseJson(const char* json) {
    jsmn_parser parser;
    jsmn_init(&parser);
    return jsmn_parse(&parser, json, strlen(json), tokens, 256);
  }

  JsonFuncs        funcs;
  NostrEventEntity event;
  jsmntok_t        tokens[256];
};

// ============================================================================
// ID Extraction Tests
// ============================================================================

TEST_F(NostrEventTest, ExtractIdSuccess) {
  const char* json = "\"aabbccdd00112233445566778899aabbccdd00112233445566778899aabbccdd\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char id[65];
  bool result = extract_nostr_event_id(&funcs, json, &tokens[0], id);
  EXPECT_TRUE(result);
  EXPECT_STREQ(id, "aabbccdd00112233445566778899aabbccdd00112233445566778899aabbccdd");
}

TEST_F(NostrEventTest, ExtractIdWrongLength) {
  const char* json = "\"aabbccdd\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char id[65];
  bool result = extract_nostr_event_id(&funcs, json, &tokens[0], id);
  EXPECT_FALSE(result);
}

TEST_F(NostrEventTest, ExtractIdNotHex) {
  const char* json = "\"gghhiijj00112233445566778899aabbccdd00112233445566778899aabbccdd\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char id[65];
  bool result = extract_nostr_event_id(&funcs, json, &tokens[0], id);
  EXPECT_FALSE(result);
}

// ============================================================================
// Pubkey Extraction Tests
// ============================================================================

TEST_F(NostrEventTest, ExtractPubkeySuccess) {
  const char* json = "\"1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char pubkey[65];
  bool result = extract_nostr_event_pubkey(&funcs, json, &tokens[0], pubkey);
  EXPECT_TRUE(result);
  EXPECT_STREQ(pubkey, "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
}

// ============================================================================
// Signature Extraction Tests
// ============================================================================

TEST_F(NostrEventTest, ExtractSigSuccess) {
  const char* json = "\"aabbccdd00112233445566778899aabbccdd00112233445566778899aabbccddaabbccdd00112233445566778899aabbccdd00112233445566778899aabbccdd\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char sig[129];
  bool result = extract_nostr_event_sig(&funcs, json, &tokens[0], sig);
  EXPECT_TRUE(result);
  EXPECT_EQ(strlen(sig), 128u);
}

// ============================================================================
// Content Extraction Tests
// ============================================================================

TEST_F(NostrEventTest, ExtractContentSimple) {
  const char* json = "\"Hello, World!\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char content[1024];
  bool result = extract_nostr_event_content(&funcs, json, &tokens[0], content, sizeof(content));
  EXPECT_TRUE(result);
  EXPECT_STREQ(content, "Hello, World!");
}

TEST_F(NostrEventTest, ExtractContentWithNewline) {
  const char* json = "\"Line1\\nLine2\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char content[1024];
  bool result = extract_nostr_event_content(&funcs, json, &tokens[0], content, sizeof(content));
  EXPECT_TRUE(result);
  EXPECT_STREQ(content, "Line1\nLine2");
}

TEST_F(NostrEventTest, ExtractContentWithTab) {
  const char* json = "\"Col1\\tCol2\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char content[1024];
  bool result = extract_nostr_event_content(&funcs, json, &tokens[0], content, sizeof(content));
  EXPECT_TRUE(result);
  EXPECT_STREQ(content, "Col1\tCol2");
}

TEST_F(NostrEventTest, ExtractContentWithQuote) {
  const char* json = "\"He said \\\"hello\\\"\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char content[1024];
  bool result = extract_nostr_event_content(&funcs, json, &tokens[0], content, sizeof(content));
  EXPECT_TRUE(result);
  EXPECT_STREQ(content, "He said \"hello\"");
}

TEST_F(NostrEventTest, ExtractContentWithBackslash) {
  const char* json = "\"path\\\\to\\\\file\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char content[1024];
  bool result = extract_nostr_event_content(&funcs, json, &tokens[0], content, sizeof(content));
  EXPECT_TRUE(result);
  EXPECT_STREQ(content, "path\\to\\file");
}

TEST_F(NostrEventTest, ExtractContentWithUnicode) {
  // \u0041 = 'A'
  const char* json = "\"Hello \\u0041\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char content[1024];
  bool result = extract_nostr_event_content(&funcs, json, &tokens[0], content, sizeof(content));
  EXPECT_TRUE(result);
  EXPECT_STREQ(content, "Hello A");
}

TEST_F(NostrEventTest, ExtractContentEmpty) {
  const char* json = "\"\"";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  char content[1024];
  bool result = extract_nostr_event_content(&funcs, json, &tokens[0], content, sizeof(content));
  EXPECT_TRUE(result);
  EXPECT_STREQ(content, "");
}

// ============================================================================
// Tags Extraction Tests
// ============================================================================

TEST_F(NostrEventTest, ExtractTagsEmpty) {
  const char* json = "[]";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  NostrTagEntity tags[10];
  uint32_t tag_count = 0;
  bool result = extract_nostr_event_tags(&funcs, json, &tokens[0], tags, &tag_count);
  EXPECT_TRUE(result);
  EXPECT_EQ(tag_count, 0u);
}

TEST_F(NostrEventTest, ExtractTagsSingleTag) {
  const char* json = "[[\"e\", \"event_id_here\"]]";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  NostrTagEntity tags[10];
  uint32_t tag_count = 0;
  bool result = extract_nostr_event_tags(&funcs, json, &tokens[0], tags, &tag_count);
  EXPECT_TRUE(result);
  EXPECT_EQ(tag_count, 1u);
  EXPECT_STREQ(tags[0].key, "e");
  EXPECT_EQ(tags[0].item_count, 1u);
  EXPECT_STREQ(tags[0].values[0], "event_id_here");
}

TEST_F(NostrEventTest, ExtractTagsMultipleValues) {
  const char* json = "[[\"e\", \"event_id\", \"relay_url\", \"marker\"]]";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  NostrTagEntity tags[10];
  uint32_t tag_count = 0;
  bool result = extract_nostr_event_tags(&funcs, json, &tokens[0], tags, &tag_count);
  EXPECT_TRUE(result);
  EXPECT_EQ(tag_count, 1u);
  EXPECT_STREQ(tags[0].key, "e");
  EXPECT_EQ(tags[0].item_count, 3u);
  EXPECT_STREQ(tags[0].values[0], "event_id");
  EXPECT_STREQ(tags[0].values[1], "relay_url");
  EXPECT_STREQ(tags[0].values[2], "marker");
}

TEST_F(NostrEventTest, ExtractTagsMultipleTags) {
  const char* json = "[[\"e\", \"event_id\"], [\"p\", \"pubkey_here\"], [\"t\", \"hashtag\"]]";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  NostrTagEntity tags[10];
  uint32_t tag_count = 0;
  bool result = extract_nostr_event_tags(&funcs, json, &tokens[0], tags, &tag_count);
  EXPECT_TRUE(result);
  EXPECT_EQ(tag_count, 3u);

  EXPECT_STREQ(tags[0].key, "e");
  EXPECT_EQ(tags[0].item_count, 1u);
  EXPECT_STREQ(tags[0].values[0], "event_id");

  EXPECT_STREQ(tags[1].key, "p");
  EXPECT_EQ(tags[1].item_count, 1u);
  EXPECT_STREQ(tags[1].values[0], "pubkey_here");

  EXPECT_STREQ(tags[2].key, "t");
  EXPECT_EQ(tags[2].item_count, 1u);
  EXPECT_STREQ(tags[2].values[0], "hashtag");
}

// ============================================================================
// Kind Extraction Tests
// ============================================================================

TEST_F(NostrEventTest, ExtractKindSuccess) {
  const char* json = "1";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  uint32_t kind = 0;
  bool result = extract_nostr_event_kind(&funcs, json, &tokens[0], &kind);
  EXPECT_TRUE(result);
  EXPECT_EQ(kind, 1u);
}

TEST_F(NostrEventTest, ExtractKindLarge) {
  const char* json = "30023";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  uint32_t kind = 0;
  bool result = extract_nostr_event_kind(&funcs, json, &tokens[0], &kind);
  EXPECT_TRUE(result);
  EXPECT_EQ(kind, 30023u);
}

// ============================================================================
// Created At Extraction Tests
// ============================================================================

TEST_F(NostrEventTest, ExtractCreatedAtSuccess) {
  const char* json = "1704067200";
  int token_count = parseJson(json);
  ASSERT_GT(token_count, 0);

  uint64_t created_at = 0;
  bool result = extract_nostr_event_created_at(&funcs, json, &tokens[0], &created_at);
  EXPECT_TRUE(result);
  EXPECT_EQ(created_at, 1704067200ull);
}
