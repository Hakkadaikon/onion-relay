#ifndef NOSTR_SERVER_LOOP_OPCODE_HANDLE_H_
#define NOSTR_SERVER_LOOP_OPCODE_HANDLE_H_

#include "../../../util/allocator.h"
#include "../../websocket_local.h"

static inline int32_t opcode_handle(
  const int32_t       client_sock,
  PWebSocketRawBuffer buffer,
  PWebSocketCallbacks callbacks,
  PWebSocketEntity    entity)
{
  switch (entity->opcode) {
    case WEBSOCKET_OP_CODE_TEXT:
    case WEBSOCKET_OP_CODE_BINARY:
      if (!is_null(callbacks->receive_callback)) {
        callbacks->receive_callback(client_sock, entity, buffer->capacity, buffer->response);
      }

      break;
    case WEBSOCKET_OP_CODE_CLOSE:
      return WEBSOCKET_ERRORCODE_SOCKET_CLOSE_ERROR;
    case WEBSOCKET_OP_CODE_PING: {
      entity->mask   = 0;
      entity->opcode = WEBSOCKET_OP_CODE_PONG;

      size_t packet_size = to_websocket_packet(entity, buffer->capacity, buffer->response);
      if (packet_size == 0) {
        log_error("Failed to create pong frame.\n");
        return WEBSOCKET_ERRORCODE_SOCKET_CLOSE_ERROR;
      }

      int32_t rtn = websocket_send(client_sock, packet_size, buffer->response);
      if (rtn != WEBSOCKET_ERRORCODE_NONE) {
        return rtn;
      }

      break;
    }
    case WEBSOCKET_OP_CODE_PONG:
      break;
    default:
      var_error("Unknown op code: ", entity->opcode);
      return WEBSOCKET_ERRORCODE_SOCKET_CLOSE_ERROR;
  }

  return WEBSOCKET_ERRORCODE_NONE;
}

#endif
