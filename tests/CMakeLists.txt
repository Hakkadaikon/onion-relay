cmake_minimum_required(VERSION 3.14)
project(websocket-test LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  websocket-test
  crypto/base64.cpp
  crypto/sha1.cpp
  http/request.cpp
   ../src/http/http.c
   ../src/crypto/sha1.c
)

add_executable(
  db-test
  nostr/db/db_tags_test.cpp
  nostr/db/db_file_test.cpp
  nostr/db/db_mmap_test.cpp
  nostr/db/db_init_test.cpp
  nostr/db/db_event_test.cpp
  nostr/db/db_index_test.cpp
  nostr/db/db_query_test.cpp
  ../src/nostr/db/db_tags.c
  ../src/nostr/db/db_file.c
  ../src/nostr/db/db_mmap.c
  ../src/nostr/db/db_init.c
  ../src/nostr/db/db_event.c
  ../src/nostr/db/index/db_index_id.c
  ../src/nostr/db/index/db_index_timeline.c
  ../src/nostr/db/index/db_index_kind.c
  ../src/nostr/db/index/db_index_pubkey.c
  ../src/nostr/db/index/db_index_pubkey_kind.c
  ../src/nostr/db/index/db_index_tag.c
  ../src/nostr/db/query/db_query.c
  ../src/nostr/db/query/db_query_result.c
  ../src/util/log.c
  ../src/arch/linux/x86_64/asm_syscall.S
)

target_include_directories(
  websocket-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_link_libraries(
  websocket-test
  GTest::gtest_main
)

target_include_directories(
  db-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_link_libraries(
  db-test
  GTest::gtest_main
)

add_executable(
  event-test
  nostr/event/nostr_event_test.cpp
  ../src/nostr/event/nostr_event.c
  ../src/nostr/event/nostr_event_id.c
  ../src/nostr/event/nostr_event_pubkey.c
  ../src/nostr/event/nostr_event_kind.c
  ../src/nostr/event/nostr_event_created_at.c
  ../src/nostr/event/nostr_event_sig.c
  ../src/nostr/event/nostr_event_tags.c
  ../src/nostr/event/nostr_event_content.c
  ../src/json/json_wrapper.c
  ../src/util/log.c
  ../src/arch/linux/x86_64/asm_syscall.S
)

target_include_directories(
  event-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_link_libraries(
  event-test
  GTest::gtest_main
)

add_executable(
  subscription-test
  nostr/subscription/nostr_subscription_test.cpp
  ../src/nostr/subscription/nostr_filter.c
  ../src/nostr/subscription/nostr_filter_ids.c
  ../src/nostr/subscription/nostr_filter_authors.c
  ../src/nostr/subscription/nostr_filter_kinds.c
  ../src/nostr/subscription/nostr_filter_since.c
  ../src/nostr/subscription/nostr_filter_until.c
  ../src/nostr/subscription/nostr_filter_limit.c
  ../src/nostr/subscription/nostr_filter_tags.c
  ../src/nostr/subscription/nostr_req.c
  ../src/nostr/subscription/nostr_close.c
  ../src/nostr/subscription/nostr_subscription.c
  ../src/json/json_wrapper.c
  ../src/util/log.c
  ../src/arch/linux/x86_64/asm_syscall.S
)

target_include_directories(
  subscription-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_link_libraries(
  subscription-test
  GTest::gtest_main
)

gtest_discover_tests(websocket-test)
gtest_discover_tests(db-test)
gtest_discover_tests(event-test)
gtest_discover_tests(subscription-test)
