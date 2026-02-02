#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

extern "C" {

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

// Function declarations
bool nostr_response_event(
  const char*             subscription_id,
  const NostrEventEntity* event,
  char*                   buffer,
  size_t                  capacity);

bool nostr_response_eose(
  const char* subscription_id,
  char*       buffer,
  size_t      capacity);

bool nostr_response_ok(
  const char* event_id,
  bool        success,
  const char* message,
  char*       buffer,
  size_t      capacity);

bool nostr_response_notice(
  const char* message,
  char*       buffer,
  size_t      capacity);

bool nostr_response_closed(
  const char* subscription_id,
  const char* message,
  char*       buffer,
  size_t      capacity);

}  // extern "C"

class NostrResponseTest : public ::testing::Test {
protected:
  void SetUp() override {
    memset(buffer, 0, sizeof(buffer));
    memset(&event, 0, sizeof(event));
  }

  char             buffer[4096];
  NostrEventEntity event;
};

// ============================================================================
// EOSE Response Tests
// ============================================================================
TEST_F(NostrResponseTest, Eose_SimpleSubscriptionId) {
  bool result = nostr_response_eose("sub1", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"EOSE\",\"sub1\"]");
}

TEST_F(NostrResponseTest, Eose_LongSubscriptionId) {
  const char* sub_id = "this-is-a-very-long-subscription-id-for-testing";
  bool result = nostr_response_eose(sub_id, buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  std::string expected = "[\"EOSE\",\"";
  expected += sub_id;
  expected += "\"]";
  EXPECT_STREQ(buffer, expected.c_str());
}

TEST_F(NostrResponseTest, Eose_SpecialCharsInSubscriptionId) {
  bool result = nostr_response_eose("sub\"with\\quotes", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"EOSE\",\"sub\\\"with\\\\quotes\"]");
}

TEST_F(NostrResponseTest, Eose_BufferTooSmall) {
  char small_buffer[10];
  bool result = nostr_response_eose("subscription", small_buffer, sizeof(small_buffer));
  EXPECT_FALSE(result);
}

TEST_F(NostrResponseTest, Eose_NullSubscriptionId) {
  bool result = nostr_response_eose(nullptr, buffer, sizeof(buffer));
  EXPECT_FALSE(result);
}

TEST_F(NostrResponseTest, Eose_NullBuffer) {
  bool result = nostr_response_eose("sub1", nullptr, 100);
  EXPECT_FALSE(result);
}

// ============================================================================
// OK Response Tests
// ============================================================================
TEST_F(NostrResponseTest, Ok_SuccessWithMessage) {
  const char* event_id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  bool result = nostr_response_ok(event_id, true, "duplicate:", buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  std::string expected = "[\"OK\",\"";
  expected += event_id;
  expected += "\",true,\"duplicate:\"]";
  EXPECT_STREQ(buffer, expected.c_str());
}

TEST_F(NostrResponseTest, Ok_FailureWithMessage) {
  const char* event_id = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
  bool result = nostr_response_ok(event_id, false, "error: invalid signature", buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  std::string expected = "[\"OK\",\"";
  expected += event_id;
  expected += "\",false,\"error: invalid signature\"]";
  EXPECT_STREQ(buffer, expected.c_str());
}

TEST_F(NostrResponseTest, Ok_EmptyMessage) {
  const char* event_id = "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
  bool result = nostr_response_ok(event_id, true, "", buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  std::string expected = "[\"OK\",\"";
  expected += event_id;
  expected += "\",true,\"\"]";
  EXPECT_STREQ(buffer, expected.c_str());
}

TEST_F(NostrResponseTest, Ok_NullMessage) {
  const char* event_id = "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd";
  bool result = nostr_response_ok(event_id, true, nullptr, buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  std::string expected = "[\"OK\",\"";
  expected += event_id;
  expected += "\",true,\"\"]";
  EXPECT_STREQ(buffer, expected.c_str());
}

TEST_F(NostrResponseTest, Ok_MessageWithSpecialChars) {
  const char* event_id = "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
  bool result = nostr_response_ok(event_id, false, "error: \"quote\" and \\slash", buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  std::string expected = "[\"OK\",\"";
  expected += event_id;
  expected += "\",false,\"error: \\\"quote\\\" and \\\\slash\"]";
  EXPECT_STREQ(buffer, expected.c_str());
}

TEST_F(NostrResponseTest, Ok_NullEventId) {
  bool result = nostr_response_ok(nullptr, true, "message", buffer, sizeof(buffer));
  EXPECT_FALSE(result);
}

// ============================================================================
// NOTICE Response Tests
// ============================================================================
TEST_F(NostrResponseTest, Notice_SimpleMessage) {
  bool result = nostr_response_notice("Hello, World!", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"NOTICE\",\"Hello, World!\"]");
}

TEST_F(NostrResponseTest, Notice_MessageWithNewline) {
  bool result = nostr_response_notice("Line1\nLine2", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"NOTICE\",\"Line1\\nLine2\"]");
}

TEST_F(NostrResponseTest, Notice_MessageWithTab) {
  bool result = nostr_response_notice("Tab\there", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"NOTICE\",\"Tab\\there\"]");
}

TEST_F(NostrResponseTest, Notice_MessageWithCarriageReturn) {
  bool result = nostr_response_notice("CR\rHere", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"NOTICE\",\"CR\\rHere\"]");
}

TEST_F(NostrResponseTest, Notice_EmptyMessage) {
  bool result = nostr_response_notice("", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"NOTICE\",\"\"]");
}

TEST_F(NostrResponseTest, Notice_NullMessage) {
  bool result = nostr_response_notice(nullptr, buffer, sizeof(buffer));
  EXPECT_FALSE(result);
}

TEST_F(NostrResponseTest, Notice_BufferTooSmall) {
  char small_buffer[15];
  bool result = nostr_response_notice("This is a long notice message", small_buffer, sizeof(small_buffer));
  EXPECT_FALSE(result);
}

// ============================================================================
// CLOSED Response Tests
// ============================================================================
TEST_F(NostrResponseTest, Closed_SimpleMessage) {
  bool result = nostr_response_closed("sub1", "subscription closed", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"CLOSED\",\"sub1\",\"subscription closed\"]");
}

TEST_F(NostrResponseTest, Closed_EmptyMessage) {
  bool result = nostr_response_closed("sub2", "", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"CLOSED\",\"sub2\",\"\"]");
}

TEST_F(NostrResponseTest, Closed_NullMessage) {
  bool result = nostr_response_closed("sub3", nullptr, buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"CLOSED\",\"sub3\",\"\"]");
}

TEST_F(NostrResponseTest, Closed_SpecialCharsInMessage) {
  bool result = nostr_response_closed("sub4", "error: \"bad\"", buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(buffer, "[\"CLOSED\",\"sub4\",\"error: \\\"bad\\\"\"]");
}

TEST_F(NostrResponseTest, Closed_NullSubscriptionId) {
  bool result = nostr_response_closed(nullptr, "message", buffer, sizeof(buffer));
  EXPECT_FALSE(result);
}

// ============================================================================
// EVENT Response Tests
// ============================================================================
TEST_F(NostrResponseTest, Event_SimpleEvent) {
  strcpy(event.id, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  strcpy(event.pubkey, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  event.kind = 1;
  event.created_at = 1704067200;
  event.tag_count = 0;
  strcpy(event.content, "Hello, Nostr!");
  strcpy(event.sig, "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");

  bool result = nostr_response_event("sub1", &event, buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  // Check that it starts correctly
  EXPECT_TRUE(strstr(buffer, "[\"EVENT\",\"sub1\",{") != nullptr);
  // Check required fields
  EXPECT_TRUE(strstr(buffer, "\"id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"") != nullptr);
  EXPECT_TRUE(strstr(buffer, "\"pubkey\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"") != nullptr);
  EXPECT_TRUE(strstr(buffer, "\"kind\":1") != nullptr);
  EXPECT_TRUE(strstr(buffer, "\"created_at\":1704067200") != nullptr);
  EXPECT_TRUE(strstr(buffer, "\"content\":\"Hello, Nostr!\"") != nullptr);
  EXPECT_TRUE(strstr(buffer, "\"tags\":[]") != nullptr);
  // Check it ends correctly
  EXPECT_TRUE(strstr(buffer, "}]") != nullptr);
}

TEST_F(NostrResponseTest, Event_WithTags) {
  strcpy(event.id, "1111111111111111111111111111111111111111111111111111111111111111");
  strcpy(event.pubkey, "2222222222222222222222222222222222222222222222222222222222222222");
  event.kind = 1;
  event.created_at = 1704067200;

  // Add a tag
  strcpy(event.tags[0].key, "e");
  strcpy(event.tags[0].values[0], "3333333333333333333333333333333333333333333333333333333333333333");
  event.tags[0].item_count = 1;
  event.tag_count = 1;

  strcpy(event.content, "Test");
  strcpy(event.sig, "4444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444");

  bool result = nostr_response_event("test-sub", &event, buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  // Check tag is present
  EXPECT_TRUE(strstr(buffer, "\"tags\":[[\"e\",\"3333333333333333333333333333333333333333333333333333333333333333\"]]") != nullptr);
}

TEST_F(NostrResponseTest, Event_WithMultipleTags) {
  strcpy(event.id, "5555555555555555555555555555555555555555555555555555555555555555");
  strcpy(event.pubkey, "6666666666666666666666666666666666666666666666666666666666666666");
  event.kind = 1;
  event.created_at = 1704067200;

  // Add tags
  strcpy(event.tags[0].key, "e");
  strcpy(event.tags[0].values[0], "event1");
  event.tags[0].item_count = 1;

  strcpy(event.tags[1].key, "p");
  strcpy(event.tags[1].values[0], "pubkey1");
  event.tags[1].item_count = 1;

  event.tag_count = 2;

  strcpy(event.content, "Multi-tag test");
  strcpy(event.sig, "7777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777");

  bool result = nostr_response_event("multi", &event, buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  // Check both tags are present
  EXPECT_TRUE(strstr(buffer, "[\"e\",\"event1\"]") != nullptr);
  EXPECT_TRUE(strstr(buffer, "[\"p\",\"pubkey1\"]") != nullptr);
}

TEST_F(NostrResponseTest, Event_ContentWithSpecialChars) {
  strcpy(event.id, "8888888888888888888888888888888888888888888888888888888888888888");
  strcpy(event.pubkey, "9999999999999999999999999999999999999999999999999999999999999999");
  event.kind = 1;
  event.created_at = 1704067200;
  event.tag_count = 0;
  strcpy(event.content, "Line1\nLine2\t\"quoted\"\\backslash");
  strcpy(event.sig, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

  bool result = nostr_response_event("escape-test", &event, buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  // Check escaped content
  EXPECT_TRUE(strstr(buffer, "\"content\":\"Line1\\nLine2\\t\\\"quoted\\\"\\\\backslash\"") != nullptr);
}

TEST_F(NostrResponseTest, Event_NullSubscriptionId) {
  strcpy(event.id, "test");
  bool result = nostr_response_event(nullptr, &event, buffer, sizeof(buffer));
  EXPECT_FALSE(result);
}

TEST_F(NostrResponseTest, Event_NullEvent) {
  bool result = nostr_response_event("sub1", nullptr, buffer, sizeof(buffer));
  EXPECT_FALSE(result);
}

TEST_F(NostrResponseTest, Event_NullBuffer) {
  strcpy(event.id, "test");
  bool result = nostr_response_event("sub1", &event, nullptr, 100);
  EXPECT_FALSE(result);
}

TEST_F(NostrResponseTest, Event_KindZero) {
  strcpy(event.id, "0000000000000000000000000000000000000000000000000000000000000000");
  strcpy(event.pubkey, "1111111111111111111111111111111111111111111111111111111111111111");
  event.kind = 0;  // Metadata kind
  event.created_at = 1704067200;
  event.tag_count = 0;
  strcpy(event.content, "{\"name\":\"test\"}");
  strcpy(event.sig, "2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222");

  bool result = nostr_response_event("metadata", &event, buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_TRUE(strstr(buffer, "\"kind\":0") != nullptr);
}

TEST_F(NostrResponseTest, Event_LargeKind) {
  strcpy(event.id, "abcdef01234567890abcdef01234567890abcdef01234567890abcdef012345");
  strcpy(event.pubkey, "fedcba9876543210fedcba9876543210fedcba9876543210fedcba98765432");
  event.kind = 30023;  // Long-form content
  event.created_at = 1704067200;
  event.tag_count = 0;
  strcpy(event.content, "Long form content");
  strcpy(event.sig, "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

  bool result = nostr_response_event("long-form", &event, buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_TRUE(strstr(buffer, "\"kind\":30023") != nullptr);
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================
TEST_F(NostrResponseTest, Eose_ExactBufferSize) {
  // Calculate exact size needed: ["EOSE","sub1"] = 15 chars + null = 16
  char exact_buffer[16];
  bool result = nostr_response_eose("sub1", exact_buffer, sizeof(exact_buffer));
  EXPECT_TRUE(result);
  EXPECT_STREQ(exact_buffer, "[\"EOSE\",\"sub1\"]");
}

TEST_F(NostrResponseTest, Ok_ZeroCapacity) {
  bool result = nostr_response_ok("id", true, "msg", buffer, 0);
  EXPECT_FALSE(result);
}

TEST_F(NostrResponseTest, Notice_ZeroCapacity) {
  bool result = nostr_response_notice("msg", buffer, 0);
  EXPECT_FALSE(result);
}

TEST_F(NostrResponseTest, Event_TagWithMultipleValues) {
  strcpy(event.id, "multivalue1111111111111111111111111111111111111111111111111111");
  strcpy(event.pubkey, "multivalue2222222222222222222222222222222222222222222222222222");
  event.kind = 1;
  event.created_at = 1704067200;

  // Add a tag with multiple values
  strcpy(event.tags[0].key, "e");
  strcpy(event.tags[0].values[0], "event_id");
  strcpy(event.tags[0].values[1], "relay_url");
  strcpy(event.tags[0].values[2], "marker");
  event.tags[0].item_count = 3;
  event.tag_count = 1;

  strcpy(event.content, "Test");
  strcpy(event.sig, "multivalue3333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333");

  bool result = nostr_response_event("multi-val", &event, buffer, sizeof(buffer));
  EXPECT_TRUE(result);

  // Check tag with multiple values
  EXPECT_TRUE(strstr(buffer, "[\"e\",\"event_id\",\"relay_url\",\"marker\"]") != nullptr);
}

TEST_F(NostrResponseTest, Event_EmptyContent) {
  strcpy(event.id, "emptycontent111111111111111111111111111111111111111111111111111");
  strcpy(event.pubkey, "emptycontent222222222222222222222222222222222222222222222222222");
  event.kind = 1;
  event.created_at = 1704067200;
  event.tag_count = 0;
  event.content[0] = '\0';  // Empty content
  strcpy(event.sig, "emptycontent333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333");

  bool result = nostr_response_event("empty", &event, buffer, sizeof(buffer));
  EXPECT_TRUE(result);
  EXPECT_TRUE(strstr(buffer, "\"content\":\"\"") != nullptr);
}
