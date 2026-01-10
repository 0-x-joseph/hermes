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

typedef struct {
  uint64_t count;
  uint64_t sum_latency;
  uint64_t max_latency;
} bench_stats_t;

void on_heartbeat(const void *msg, void *user_data) {
  bench_stats_t *stats = (bench_stats_t *)user_data;
  const Heartbeat *hb = (const Heartbeat *)msg;

  uint64_t now = get_time_us();
  uint64_t latency = now - hb->timestamp_us;

  stats->count++;
  stats->sum_latency += latency;
  if (latency > stats->max_latency)
    stats->max_latency = latency;

  if (stats->count % 100 == 0) {
    printf(
        "📊 [Node %d] Count: %lu | Avg: %lu us | Max: %lu us | CPU: %.1f%%\n",
        hb->node_id, stats->count, stats->sum_latency / stats->count,
        stats->max_latency, hb->cpu_usage);
  }
}

int main() {
  hermes_t *h = hermes_init("nats://127.0.0.1:4222", 1024 * 1024);
  bench_stats_t stats = {0, 0, 0};

  printf("📡 Hermes Subscriber listening on 'hermes.drone.1.hb'...\n");
  hermes_subscribe(h, "hermes.drone.1.hb", &heartbeat__descriptor, on_heartbeat,
                   &stats);

  while (1) {
    sleep(1); // Keep main thread alive
  }

  hermes_destroy(h);
  return 0;
}
