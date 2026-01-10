# Project Hermes (Alpha Prototype) 🚀

**Hermes** is a high-performance, single-header C framework designed to replace ROS2 in high-frequency swarm robotics. This is currently an **Alpha Prototype** demonstrating that ultra-low latency communication is possible using a deterministic memory-pooled architecture.

### ⏱ The "Sub-Millisecond" Benchmark

In its current prototype state, Hermes achieves a consistent ** End-to-End latency** for inter-process communication (IPC) at 100Hz.

---

## 📊 Prototype Architecture

Hermes addresses the "Latency Jitter" problem in robotics by combining a high-speed transport with a zero-allocation memory strategy.

* **Transport Layer:** NATS (C-Client) for lightweight, broker-based messaging.
* **Serialization:** Protobuf-C for deterministic binary packing.
* **Memory Strategy:** A **Pre-allocated Shared Memory Pool** (Circular Buffer). By avoiding `malloc` during the execution loop, we eliminate the primary source of timing spikes found in ROS2/DDS.

---

## 📊 Performance Data

Current results from the multi-process benchmark (Publisher  NATS  Subscriber):

* **Average Latency:** 
* **Target:**  (Planned for Beta)
* **Jitter:** Minimal (Max spikes stay within  on standard Linux kernels).

---

## 🚀 How to Run the Prototype

### 1. Prerequisites

Ensure you have the NATS server and the Protobuf-C compiler:

```bash
sudo apt update && sudo apt install libprotobuf-c-dev protobuf-c-compiler nats-server

```

### 2. Build

Hermes uses Git submodules to ensure a "vendored" build environment (no external dynamic dependencies).

```bash
git clone --recursive [your-repo-url]
cd hermes
mkdir build && cd build
cmake ..
make

```

### 3. Execution

Open three terminals to simulate a drone-to-ground-station environment:

1. **Terminal 1:** `nats-server` (The message bus)
2. **Terminal 2:** `./hermes_sub` (The listener/metrics engine)
3. **Terminal 3:** `./hermes_pub` (The flight controller simulator)

---
