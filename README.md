# Computer-Architecture-Simulators
A collection of high-performance microarchitectural simulators implemented in C++, covering Branch Prediction, Cache Memory hierarchies, Data Dependency analysis, and Multithreading architectures. 

# 🏛️ Advanced Computer Architecture Simulators

This repository contains a suite of high-performance microarchitectural simulators implemented from scratch in **C++**. These projects focus on modeling, analyzing, and optimizing hardware components critical to modern CPU design and execution efficiency.

Each directory represents a specific architectural simulation designed to evaluate performance trade-offs under various workloads (instruction traces).

---

## 🛠️ Simulators Overview

### 1. 🎯 Dynamic Branch Predictor Simulator (`/branch-predictor`)
Models a flexible **Two-Level Branch Predictor** (local/global history registers and pattern history tables) to forecast conditional branch outcomes.
* **Key Features:** Supports customizable global/local history configurations, shared/local predictor tables, and dynamic state updates based on runtime execution traces.
* **Impact:** Demonstrates understanding of speculative execution, pipeline stalls, and branch misprediction penalties.

### 2. 💾 Cache Hierarchy Simulator (`/cache-simulator`)
A simulator that models a multi-level cache memory subsystem (**L1 and L2 caches**) to analyze memory access patterns and latency.
* **Key Features:** Configurable cache sizes, associativity, and block sizes. Supports **Write-Allocate / Write-Back** policies, **Inclusive** cache properties, and an **LRU (Least Recently Used)** eviction mechanism.
* **Impact:** Outputs Hit/Miss rates and Average Memory Access Time (AMAT), showcasing insights into memory wall bottlenecks and spatial/temporal locality.

### 3. 📊 Data Dependency & Parallelism Analyzer (`/dependency-analyzer`)
Analyzes program instruction traces to map out true data dependencies (**Read-After-Write / RAW**) to determine theoretical instruction-level parallelism (ILP).
* **Key Features:** Constructs a dynamic data dependency graph from an execution trace to calculate the minimum depth/critical path of execution.
* **Impact:** Essential for understanding **Out-of-Order (OoO) Execution** engines, register renaming limits, and hardware resource allocation.

### 4. 🧵 Multithreading Processor Core Simulator (`/multithreading-sim`)
Simulates the execution of trace workloads on a multi-threaded CPU core supporting two distinct microarchitectural paradigms:
* **Fine-Grained Multithreading:** Context switches between threads every clock cycle.
* **Block Multithreading:** Context switches only when encountering high-latency events (e.g., a memory `LOAD`/`STORE` miss penalty).
* **Impact:** Explores thread-level parallelism (TLP), hardware context management, and cycle-by-cycle pipeline throughput optimization.

---

## 🚀 Technologies & Requirements

* **Language:** C
* **Build System:** Make / GCC Linux environment
* **Inputs:** Accepts standard architectural simulation trace files (`.trace` / text logs containing PC addresses and memory actions).

---
