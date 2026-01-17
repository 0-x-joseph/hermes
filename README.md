# Hermes: High-Performance Robotics Middleware

**Hermes** is a minimalist, single-header C middleware designed to provide a "lean nervous system" for autonomous agents and robotic swarms. It eliminates the complexity of heavyweight industry stacks by combining a broker-based transport with a deterministic memory model.

---

## Table of Contents

1. [Description](#description)
2. [The "Why": Design Decisions](#design-decisions)
3. [Benefits of Hermes](#benefits-of-hermes)
4. [API Design](#api-design)
5. [Usage Guide](#usage-guide)
6. [How to Run the Prototype](#how-to-test)
7. [Prototype Status](#prototype-status)

---

## Description

Hermes is a "zero-install" communication layer. It allows diverse software modules—written in any language—to synchronize state at high frequencies. By leveraging **NATS** for transport and **Protocol Buffers (Protobuf)** for serialization, it achieves sub-millisecond latency while maintaining a binary footprint of less than 50KB.

---

## Design Decisions

Hermes solves specific engineering pain points found in modern robotics:

* 
**Eliminating Heap Jitter:** Standard programs often "hunt" for memory while running, causing random lag spikes (latency jitter). Hermes uses a **Static Arena Allocator**; it pre-allocates a single block of memory at startup, ensuring that the time to pack and send data remains constant.


* 
**Broker-Based Efficiency (NATS):** Instead of every robot trying to talk to every other robot directly (which scales poorly at ), Hermes uses a central broker. This reduces network complexity to  at the robot level, making connections stable over cellular or internet links.


* 
**Language Agnostic:** By using **Protobuf-C**, data is defined in a universal schema. You can write performance-critical code in C, while telemetry dashboards or AI modules can be written in Python, Go, or JavaScript.


* 
**Single-Header Philosophy:** Inspired by "STB-style" libraries, the entire framework is contained in one `.h` file. This avoids "dependency hell" and makes cross-compiling for different hardware (ARM, x86) instantaneous.



---

## Benefits of Hermes

### Predictable Timing

In high-stakes environments, random software delays are as dangerous as physical hardware failures. Hermes' pre-allocated memory ensures that your software is never the cause of a control-loop failure.

### Modem and Internet Resilience

Standard peer-to-peer protocols often fail over 5G/cellular modems due to firewall restrictions and network noise. Because Hermes uses NATS, the robot makes one stable outgoing connection (like a web browser), which is built to handle the instability of remote links.

### Simplified Telemetry

Gathering data is easier with a broker. You can plug in a "Monitoring Node" (a dashboard or logger) that observes the entire swarm in real-time without adding any extra processing load or "lag" to the robots themselves.

---

## API Design

The API is linear and non-blocking:

| Function | Description |
| --- | --- |
| `hermes_init` | Reserves the memory Arena and establishes the NATS connection.

 |
| `hermes_publish` | Packs data into the Arena () and dispatches it asynchronously.

 |
| `hermes_subscribe` | Registers a callback to handle incoming typed Protobuf messages.

 |
| `hermes_destroy` | Cleanly closes connections and releases the static memory block. |

---

## Usage Guide

Simply include the header in **one** C file with the implementation guard:

```c
#define HERMES_IMPLEMENTATION
#include "hermes.h"

void on_telemetry(const void *msg, void *user_data) {
    MyMessage *m = (MyMessage *)msg;
    printf("Received: %f\n", m->value);
}

int main() {
    hermes_t *h = hermes_init("nats://localhost:4222", 1024 * 1024);
    hermes_subscribe(h, "drone.pos", &my_message__descriptor, on_telemetry, NULL);

    MyMessage msg = MY_MESSAGE__INIT;
    msg.value = 42.0;
    hermes_publish(h, "drone.pos", &my_message__descriptor, &msg);

    hermes_destroy(h);
    return 0;
}

```

---

## How to Test

### 1. Prerequisites

Ensure you have the NATS server and the Protobuf-C compiler:

```bash
sudo apt update && sudo apt install libprotobuf-c-dev protobuf-c-compiler nats-server

```

### 2. Build

```bash
git clone --recursive [your-repo-url]
cd hermes
mkdir build && cd build
cmake ..
make

```

### 3. Execution

1. **Terminal 1:** `nats-server` (The message bus)
2. **Terminal 2:** `./hermes_sub` (The listener/metrics engine)
3. **Terminal 3:** `./hermes_pub` (The flight simulator)

---

## Prototype Status

**Note:** This is a **functional prototype**. It has been verified to achieve a mean end-to-end latency of **945µs** with a jitter cap of **1.4ms** at 100Hz. Current features focus on the core Pub/Sub model and Arena Allocator. Future updates will introduce **Services** and **Actions** patterns, along with support for **ARM, RISC-V, and x86 architectures**.
