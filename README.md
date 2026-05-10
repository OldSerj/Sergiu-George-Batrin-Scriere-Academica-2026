# Sergiu-George-Batrin-Scriere-Academica-2026

## Sorting Algorithm Benchmark: Experimental Performance Analysis of Eleven Sorting Algorithms in C++


## Abstract
This study benchmarks eleven sorting algorithms implemented in C++17 across multiple data types, input distributions, and input sizes up to ten million elements. The results confirm theoretical expectations while revealing significant hardware‑driven effects, including cache‑miss amplification in Shell Sort, worst‑case elimination in DNF Quick Sort, strong scaling in Parallel Sort, and extreme efficiency of Counting Sort on bounded domains. The findings highlight the importance of memory hierarchy and parallelism in real‑world algorithm performance.

## Key Points
- **Algorithms tested:** 11 classical and modern sorting algorithms, including `std::sort` and Parallel Sort.
- **Data types:** `int32`, `float32`, `char`.
- **Distributions:** random, sorted, reverse‑sorted, half‑sorted, nearly sorted.
- **Scale:** 390 measurements across sizes from 20 to 10,000,000.

## Core Findings
- **Parallel Sort** achieves the best performance at large input sizes.
- **Radix Sort** is the fastest single‑threaded algorithm on integer‑like data.
- **Shell Sort (Hibbard)** collapses at scale due to cache‑unfriendly gap strides.
- **Tim Sort** benefits significantly from ordered input.
- **Counting Sort** dominates on `char` due to fixed domain size.
- **Type effects:** `float32` ≈ `int32`; `char` benefits from smaller memory footprint.

## Conclusion
Real‑world sorting performance is shaped by hardware behavior, memory access patterns, and parallel execution—not just asymptotic complexity.
