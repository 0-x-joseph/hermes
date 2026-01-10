#include "heartbeat.pb-c.h"
#include "hermes.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

uint64_t get_time_us() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

int main() {
  // Initialize Hermes with 1MB pool
  hermes_t *h = hermes_init("nats://127.0.0.1:4222", 1024 * 1024);
  if (!h)
    return 1;

  printf("🚀 Hermes Publisher [ID: 1] starting at 100Hz...\n");

  Heartbeat msg = HEARTBEAT__INIT;
  msg.node_id = 1;
  msg.cpu_usage = 15.5f; // Initial simulated value

  while (1) {
    msg.timestamp_us = get_time_us();

    // Slightly vary CPU usage to simulate real data
    msg.cpu_usage += 0.1f;
    if (msg.cpu_usage > 100.0f)
      msg.cpu_usage = 10.0f;

    hermes_publish(h, "hermes.drone.1.hb", &heartbeat__descriptor, &msg);

    // 10ms delay for 100Hz
    usleep(10000);
  }

  hermes_destroy(h);
  return 0;
}
