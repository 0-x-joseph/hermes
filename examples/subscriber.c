#include "../build/heartbeat.pb-c.h"
#include "../include/hermes.h"
#include <math.h>
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
  uint64_t sum_lat;
  uint64_t max_lat;
  uint64_t min_lat;
  double M2; // For online variance calculation (Welford's algorithm)
  FILE *log_file;
} bench_stats_t;

void on_heartbeat(const void *msg, void *user_data) {
  bench_stats_t *stats = (bench_stats_t *)user_data;
  const Heartbeat *hb = (const Heartbeat *)msg;

  uint64_t now = get_time_us();
  uint64_t latency = now - hb->timestamp_us;

  stats->count++;

  // Standard Avg/Max/Min
  stats->sum_lat += latency;
  if (latency > stats->max_lat)
    stats->max_lat = latency;
  if (latency < stats->min_lat || stats->min_lat == 0)
    stats->min_lat = latency;

  // Online Variance Calculation
  double avg = (double)stats->sum_lat / stats->count;
  double delta = (double)latency - avg;
  stats->M2 +=
      delta * ((double)latency - ((double)stats->sum_lat / stats->count));

  // Log raw data for charting
  if (stats->log_file) {
    fprintf(stats->log_file, "%lu,%lu\n", stats->count, latency);
  }

  if (stats->count % 100 == 0) {
    double std_dev = sqrt(stats->M2 / stats->count);
    printf("📊 Count: %5lu | Avg: %4lu us | Max: %4lu us | Min: %4lu us | "
           "Jitter (σ): %.2f us\n",
           stats->count, stats->sum_lat / stats->count, stats->max_lat,
           stats->min_lat, std_dev);
  }
}

int main() {
  hermes_t *h = hermes_init("nats://127.0.0.1:4222", 1024 * 1024);
  bench_stats_t stats = {0, 0, 0, 0, 0.0, NULL};

  stats.log_file = fopen("latency_results.csv", "w");
  fprintf(stats.log_file, "message_index,latency_us\n");

  printf("📡 Hermes Subscriber listening with Jitter Analysis...\n");
  hermes_subscribe(h, "hermes.drone.1.hb", &heartbeat__descriptor, on_heartbeat,
                   &stats);

  while (1) {
    sleep(1);
  }

  if (stats.log_file)
    fclose(stats.log_file);
  hermes_destroy(h);
  return 0;
}
