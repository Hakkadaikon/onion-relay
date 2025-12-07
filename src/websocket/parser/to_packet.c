/**
 * @file  to_packet.c
 *
 * @brief Parses each parameter of a websocket packet stored in network byte order.
 * @see RFC6455 (https://datatracker.ietf.org/doc/html/rfc6455)
 */

#include "../../util/allocator.h"
#include "../websocket_local.h"

size_t to_websocket_packet(PWebSocketEntity restrict entity, const size_t capacity, char* restrict raw)
{
  if (!entity || !raw || capacity < 2) {
    return 0;
  }

  size_t offset = 0;

  // 0-7bit
  // +-+-+-+-+-------+
  // |F|R|R|R| opcode|
  // |I|S|S|S|  (4)  |
  // |N|V|V|V|       |
  // | |1|2|3|       |
  // +-+-+-+-+-------+
  raw[offset] = ((entity->fin & 0x01) << 7) |
                ((entity->rsv1 & 0x01) << 6) |
                ((entity->rsv2 & 0x01) << 5) |
                ((entity->rsv3 & 0x01) << 4) |
                (entity->opcode & 0x0F);
  offset++;

  // 8-15bit
  // +-+-------------+
  // |M| Payload len |
  // |A|     (7)     |
  // |S|             |
  // |K|             |
  // +-+-------------+
  unsigned char mask = (entity->mask ? 0x80 : 0x00);

  if (entity->ext_payload_len <= 125 && !entity->mask) {
    raw[offset] = entity->ext_payload_len & 0x7F;
    offset++;
    websocket_memcpy(&raw[offset], entity->payload, entity->ext_payload_len);
    return offset + entity->ext_payload_len;
  }

  if (entity->ext_payload_len > 125 && entity->ext_payload_len <= 0xFFFF) {
    raw[offset] = mask | 126;  // Mask set + payload length 126
    offset++;
    if (capacity < offset + 2) {
      return 0;
    }
    raw[offset]     = (entity->ext_payload_len >> 8) & 0xFF;
    raw[offset + 1] = entity->ext_payload_len & 0xFF;
    offset += 2;
  } else if (entity->ext_payload_len > 0xFFFF) {
    raw[offset] = mask | 127;  // Mask set + payload length 127
    offset++;
    if (capacity < offset + 8) {
      return 0;
    }
    for (int32_t i = 7; i >= 0; i--) {
      raw[offset + i] = entity->ext_payload_len & 0xFF;
      entity->ext_payload_len >>= 8;
    }
    offset += 8;
  } else {
    raw[offset] = mask | entity->ext_payload_len;
    offset++;
  }

  // Masking key (if mask is set)
  if (entity->mask) {
    if (capacity < offset + 4) {
      return 0;
    }
    websocket_memcpy(&raw[offset], entity->masking_key, 4);
    offset += 4;
  }

  // Payload data
  if (capacity < offset + entity->ext_payload_len) {
    return 0;
  }

  if (entity->mask) {
    // use loop unroll
    size_t i = 0;
    for (; i + 4 <= entity->ext_payload_len; i += 4) {
      raw[offset + i + 0] = entity->payload[i + 0] ^ entity->masking_key[0];
      raw[offset + i + 1] = entity->payload[i + 1] ^ entity->masking_key[1];
      raw[offset + i + 2] = entity->payload[i + 2] ^ entity->masking_key[2];
      raw[offset + i + 3] = entity->payload[i + 3] ^ entity->masking_key[3];
    }
    for (; i < entity->ext_payload_len; i++) {
      raw[offset + i] = entity->payload[i] ^ entity->masking_key[i % 4];
    }
  } else {
    websocket_memcpy(&raw[offset], &entity->payload[0], entity->ext_payload_len);
  }

  offset += entity->ext_payload_len;

  return offset;
}
