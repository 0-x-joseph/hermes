#include "../build/heartbeat.pb-c.h"
#include "../include/hermes.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

uint64_t get_time_us() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

int main() {
  hermes_t *h = hermes_init("nats://127.0.0.1:4222", 1024 * 1024);
  if (!h)
    return 1;

  printf("🚀 Hermes Publisher [ID: 1] starting at 100Hz...\n");

  Heartbeat msg = HEARTBEAT__INIT;
  msg.node_id = 1;
  msg.cpu_usage = 0.0f;

  uint64_t start_time = get_time_us();
  uint64_t msg_count = 0;

  while (1) {
    msg.timestamp_us = get_time_us();
    msg.cpu_usage = (float)(msg_count % 100);

    hermes_publish(h, "hermes.drone.1.hb", &heartbeat__descriptor, &msg);
    msg_count++;

    if (msg_count % 1000 == 0) {
      double elapsed = (get_time_us() - start_time) / 1000000.0;
      printf("[PUB] Total Msgs: %lu | Throughput: %.2f msg/s\n", msg_count,
             (double)msg_count / elapsed);
    }

    // 10ms interval for 100Hz
    usleep(10000);
  }

  hermes_destroy(h);
  return 0;
}
