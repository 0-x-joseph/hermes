/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hermes.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ybouryal <ybouryal@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 19:00:17 by ybouryal          #+#    #+#             */
/*   Updated: 2026/01/10 19:25:37 by ybouryal         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __HERMES__
#define __HERMES__

#include "nats.h"
#include <assert.h>
#include <protobuf-c/protobuf-c.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* forward declaration*/
typedef struct hermes_context hermes_t;

typedef void (*hermes_handler_t)(const void *msg, void *user_data);

hermes_t *hermes_init(const char *url, size_t pool_size);

void hermes_destroy(hermes_t *h);

int hermes_publish(hermes_t *h, const char *topic,
                   const ProtobufCMessageDescriptor *d, const void *msg);

int hermes_subscribe(hermes_t *h, const char *topic,
                     const ProtobufCMessageDescriptor *d, hermes_handler_t cb,
                     void *user_data);
#define HERMES_IMPLEMENTATION

#ifdef HERMES_IMPLEMENTATION

struct hermes_context {
  natsConnection *nc;
  uint8_t *pool;
  size_t pool_size;
  size_t pool_ptr;
};

typedef struct {
  hermes_handler_t user_cb;
  const ProtobufCMessageDescriptor *desc;
  void *user_data;
} _hermes_sub_info;

static void _internal_on_msg(natsConnection *nc, natsSubscription *sub,
                             natsMsg *msg, void *closure) {
  _hermes_sub_info *info = (_hermes_sub_info *)closure;

  ProtobufCMessage *unpacked =
      protobuf_c_message_unpack(info->desc, NULL, natsMsg_GetDataLength(msg),
                                (const uint8_t *)natsMsg_GetData(msg));

  if (unpacked) {
    info->user_cb(unpacked, info->user_data);
    protobuf_c_message_free_unpacked(unpacked, NULL);
  }
  natsMsg_Destroy(msg);
}

hermes_t *hermes_init(const char *url, size_t pool_size) {
  hermes_t *h = (hermes_t *)calloc(1, sizeof(hermes_t));
  assert(h != NULL);
  natsStatus ret = natsConnection_ConnectTo(&h->nc, url);
  assert(ret == NATS_OK);

  h->pool = (uint8_t *)malloc(pool_size);
  assert(h->pool != NULL);
  h->pool_size = pool_size;
  h->pool_ptr = 0;
  return h;
}

void hermes_destroy(hermes_t *h) {
  natsConnection_Destroy(h->nc);
  free(h->pool);
  free(h);
}

int hermes_publish(hermes_t *h, const char *topic,
                   const ProtobufCMessageDescriptor *d, const void *msg) {
  const ProtobufCMessage *p_msg = (const ProtobufCMessage *)msg;
  size_t len = protobuf_c_message_get_packed_size(p_msg);

  if (len > h->pool_size)
    return -1;
  if (h->pool_ptr + len > h->pool_size)
    h->pool_ptr = 0;

  uint8_t *buf = &h->pool[h->pool_ptr];
  protobuf_c_message_pack(p_msg, buf);
  h->pool_ptr += len;

  return natsConnection_Publish(h->nc, topic, (const void *)buf, (int)len);
}

int hermes_subscribe(hermes_t *h, const char *topic,
                     const ProtobufCMessageDescriptor *d, hermes_handler_t cb,
                     void *user_data) {
  _hermes_sub_info *info = (_hermes_sub_info *)malloc(sizeof(_hermes_sub_info));
  assert(info != NULL);
  info->user_cb = cb;
  info->desc = d;
  info->user_data = user_data;
  natsSubscription *sub = NULL;
  return natsConnection_Subscribe(&sub, h->nc, topic, _internal_on_msg,
                                  (void *)info);
}
#endif /* HERMES_IMPLEMENTATION */

#endif /* __HERMES__ */
