#include "json_wrapper.h"

#include "../util/string.h"
#include "jsmn.h"

static bool json_wrapper_init(PJsonParser parser)
{
  require_not_null(parser, false);

  jsmn_init(parser);
  return true;
}

static int32_t json_wrapper_parse(
  PJsonParser    parser,
  const char*    json,
  const size_t   json_len,
  jsontok_t*     tokens,
  const uint32_t num_tokens)
{
  require_not_null(parser, 0);
  require_not_null(json, 0);
  require_valid_length(json_len, 0);
  require_not_null(tokens, 0);
  require_valid_length(num_tokens, 0);

  return jsmn_parse(parser, json, json_len, tokens, num_tokens);
}

static bool json_wrapper_is_primitive(const jsontok_t* token)
{
  require_not_null(token, false);
  return (token->type == JSMN_PRIMITIVE);
}

static bool json_wrapper_is_string(const jsontok_t* token)
{
  require_not_null(token, false);
  return (token->type == JSMN_STRING);
}

static bool json_wrapper_is_array(const jsontok_t* token)
{
  require_not_null(token, false);
  return (token->type == JSMN_ARRAY);
}

static bool json_wrapper_is_object(const jsontok_t* token)
{
  require_not_null(token, false);
  return (token->type == JSMN_OBJECT);
}

static bool json_wrapper_strncmp(
  const char*      json,
  const jsontok_t* token,
  const char*      str,
  const size_t     str_len)
{
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(str, false);
  require_valid_length(str_len, false);

  if (!json_wrapper_is_string(token)) {
    return false;
  }

  if (!strncmp(json + token->start, str, str_len)) {
    return false;
  }

  return true;
}

static size_t json_wrapper_get_token_length(const jsontok_t* token)
{
  require_not_null(token, 0);
  return (token->end - token->start);
}

bool json_funcs_init(PJsonFuncs funcs)
{
  require_not_null(funcs, false);

  funcs->init             = json_wrapper_init;
  funcs->parse            = json_wrapper_parse;
  funcs->strncmp          = json_wrapper_strncmp;
  funcs->is_array         = json_wrapper_is_array;
  funcs->is_object        = json_wrapper_is_object;
  funcs->is_string        = json_wrapper_is_string;
  funcs->is_primitive     = json_wrapper_is_primitive;
  funcs->get_token_length = json_wrapper_get_token_length;

  return true;
}
