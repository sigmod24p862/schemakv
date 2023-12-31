//
//  profiler.h
//  PROJECT profiler.h
//
//  Created on 18/05/2022.
//  Copyright (c) 2022 .
//

#pragma once

#include <cpuid.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <iostream>

#include <stdint.h>  // uint64_t

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64) || defined(__i386)
#ifdef _WIN32
#include <intrin.h>  // __rdtsc
#else
#include <x86intrin.h>  // __rdtsc
#endif
#define HAS_HW_RDTSC 1
#else
#include <chrono>  // std::chrono::high_resolution_clock
#define HAS_HW_RDTSC 0
#endif

#define TSC_Hz (2095078ull * 1000)
#define NANOSEC_BASE 1000000000
#define MICROSEC_BASE 1000000

#define PERCISION (1ull << 20)

constexpr uint64_t tsc_percision = TSC_Hz / PERCISION;
constexpr uint64_t nano_percision = NANOSEC_BASE / PERCISION;

inline uint64_t getTicksByNanosecs(uint64_t nanoseconds) {
  // return ((uint64_t)(nanoseconds) * (tsc_percision)) / (nano_percision);
  return nanoseconds << 1;
}
inline uint64_t getNanoSecsByTicks(uint64_t ticks) {
  // return ((uint64_t)(ticks) * (nano_percision)) /
  //  (tsc_percision);
  return ticks >> 1;
}
inline uint64_t rte_rdtsc() {
  union {
    uint64_t tsc_64;
    struct {
      uint32_t lo_32;
      uint32_t hi_32;
    };
  } tsc;
  asm volatile("rdtsc" : "=a"(tsc.lo_32), "=d"(tsc.hi_32));
  return tsc.tsc_64;
}

inline uint64_t rdtsc() {
#if HAS_HW_RDTSC
  // _mm_lfence() might be used to serialize the instruction stream,
  // and it would guarantee that RDTSC will not be reordered with
  // other instructions.  However, measurements show that the overhead
  // may be too big (easily 15 to 30 CPU cycles) for profiling
  // purposes: if reordering matters, the overhead matters too!

  // Forbid the compiler from reordering instructions
#ifdef _MSC_VER
  _ReadWriteBarrier();
#else
  __asm__ __volatile__("" : : : "memory");
#endif

  uint64_t result = __rdtsc();

  // Forbid the compiler from reordering instructions
#ifdef _MSC_VER
  _ReadWriteBarrier();
#else
  __asm__ __volatile__("" : : : "memory");
#endif

  return result;
#else
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             now.time_since_epoch())
      .count();
#endif
}

class PointProfiler {
 public:
  inline PointProfiler() {}
  ~PointProfiler() {}
  static uint64_t now() { return rte_rdtsc(); }

  inline void start() { start_tick_ = rte_rdtsc(); }
  // return nano seconds
  inline uint64_t end() {
    uint64_t end_tick = rte_rdtsc();
    return duration_ = getNanoSecsByTicks(end_tick - start_tick_);
  }

  inline uint64_t duration() { return duration_; }

 private:
  uint64_t start_tick_ = 0;
  uint64_t duration_ = 0;
};
