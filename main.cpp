// ================================================================
//  main.cpp  –  Multi-Type Parallel Sorting Benchmark
//
//  Compile (GCC / Clang / WSL / MinGW-w64):
//    g++ -O2 -std=c++17 -pthread main.cpp -o sort_bench
//
//  Compile (MSVC, Developer Command Prompt):
//    cl /O2 /std:c++17 main.cpp
//
//  Run:
//    ./sort_bench          (Linux / macOS / WSL)
//    sort_bench.exe        (Windows)
//
//  Output:
//    sorting_results.csv
// AMD Ryzen AI 9 HX 370 w/ Radeon 890M (2.00 GHz)
// 12 cores / 24 threads
// ================================================================

#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

static constexpr int N_THREADS    = 24;
static constexpr int N2_THRESHOLD = 10'000;
static constexpr int PAR_MIN      = 2'048;

static const std::vector<int> SIZES = {
    20, 50, 100, 500,
    1'000, 5'000, 10'000,
    50'000, 100'000, 500'000,
    1'000'000, 5'000'000, 10'000'000
};

template<typename Fn>
static double measure_ms(Fn&& fn) {
    using Clock = std::chrono::high_resolution_clock;
    auto t0 = Clock::now();
    fn();
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

template<typename T>
static void insertionSort(T* a, int n) {
    for (int i = 1; i < n; ++i) {
        T key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) { a[j+1] = a[j]; --j; }
        a[j+1] = key;
    }
}
template<typename T>
static void selectionSort(T* a, int n) {
    for (int i = 0; i < n-1; ++i) {
        int m = i;
        for (int j = i+1; j < n; ++j) if (a[j] < a[m]) m = j;
        if (m != i) std::swap(a[i], a[m]);
    }
}

template<typename T>
static void shellSort(T* a, int n) {
    static const int GAPS[] = {701,301,132,57,23,10,4,1};
    for (int gap : GAPS) {
        for (int i = gap; i < n; ++i) {
            T tmp = a[i]; int j = i;
            while (j >= gap && a[j-gap] > tmp) { a[j] = a[j-gap]; j -= gap; }
            a[j] = tmp;
        }
    }
}

template<typename T>
static void qsImpl(T* a, int lo, int hi) {
    if (hi - lo < 16) { insertionSort(a + lo, hi - lo + 1); return; }

    int mid = lo + (hi - lo) / 2;
    if (a[lo]  > a[mid]) std::swap(a[lo],  a[mid]);
    if (a[lo]  > a[hi])  std::swap(a[lo],  a[hi]);
    if (a[mid] > a[hi])  std::swap(a[mid], a[hi]);
    std::swap(a[mid], a[hi-1]);
    T pivot = a[hi-1];

    int i = lo, j = hi - 1;
    for (;;) {
        while (a[++i] < pivot) {}
        while (a[--j] > pivot) {}
        if (i >= j) break;
        std::swap(a[i], a[j]);
    }
    std::swap(a[i], a[hi-1]);
    qsImpl(a, lo, i-1);
    qsImpl(a, i+1, hi);
}

template<typename T>
static void quickSort(T* a, int n) {
    if (n < 2)  return;
    if (n < 16) { insertionSort(a, n); return; }
    qsImpl(a, 0, n-1);
}

template<typename T>
static void doMerge(T* a, int l, int m, int r) {
    std::vector<T> buf(a + l, a + m + 1);
    int i = 0, j = m + 1, k = l;
    while (i < (int)buf.size() && j <= r)
        a[k++] = (buf[i] <= a[j]) ? buf[i++] : a[j++];
    while (i < (int)buf.size()) a[k++] = buf[i++];
}

template<typename T>
static void msImpl(T* a, int l, int r) {
    if (l >= r) return;
    int m = l + (r - l) / 2;
    msImpl(a, l, m);
    msImpl(a, m+1, r);
    doMerge(a, l, m, r);
}

template<typename T>
static void mergeSort(T* a, int n) {
    if (n < 2) return;
    msImpl(a, 0, n-1);
}

template<typename T>
static void siftDown(T* a, int n, int i) {
    for (;;) {
        int largest = i, l = 2*i+1, r = 2*i+2;
        if (l < n && a[l] > a[largest]) largest = l;
        if (r < n && a[r] > a[largest]) largest = r;
        if (largest == i) break;
        std::swap(a[i], a[largest]);
        i = largest;
    }
}

template<typename T>
static void heapSort(T* a, int n) {
    for (int i = n/2-1; i >= 0; --i) siftDown(a, n, i);
    for (int i = n-1;   i >  0; --i) { std::swap(a[0], a[i]); siftDown(a, i, 0); }
}

template<typename T>
static void timSort(T* a, int n) {
    const int RUN = 32;
    for (int i = 0; i < n; i += RUN)
        insertionSort(a + i, std::min(RUN, n - i));
    for (int sz = RUN; sz < n; sz *= 2) {
        for (int l = 0; l < n; l += 2*sz) {
            int m = std::min(l + sz - 1,   n - 1);
            int r = std::min(l + 2*sz - 1, n - 1);
            if (m < r) doMerge(a, l, m, r);
        }
    }
}

template<typename T>
static void stdSort(T* a, int n) { std::sort(a, a+n); }

template<typename T>
static void parallelSort(T* a, int n) {
    if (n < PAR_MIN) { std::sort(a, a+n); return; }

    int nT    = std::min(N_THREADS, n);
    int chunk = (n + nT - 1) / nT;

    {
        std::vector<std::thread> threads;
        threads.reserve(nT);
        for (int t = 0; t < nT; ++t) {
            int s = t * chunk,  e = std::min(s + chunk, n);
            if (s >= n) break;
            threads.emplace_back([=]{ std::sort(a+s, a+e); });
        }
        for (auto& th : threads) th.join();
    }

    for (int w = chunk; w < n; w *= 2) {
        std::vector<std::thread> threads;
        for (int i = 0; i < n; i += 2*w) {
            int mid = std::min(i + w, n);
            int end = std::min(i + 2*w, n);
            if (mid < end)
                threads.emplace_back([=]{ std::inplace_merge(a+i, a+mid, a+end); });
        }
        for (auto& th : threads) th.join();
    }
}

static void lsdRadix32(uint32_t* a, int n) {
    std::vector<uint32_t> buf(n);
    for (int shift = 0; shift < 32; shift += 8) {
        uint32_t cnt[256] = {};
        for (int i = 0; i < n; ++i) cnt[(a[i] >> shift) & 0xFF]++;
        uint32_t acc = 0;
        for (int i = 0; i < 256; ++i) { uint32_t c = cnt[i]; cnt[i] = acc; acc += c; }
        for (int i = 0; i < n; ++i)   buf[cnt[(a[i] >> shift) & 0xFF]++] = a[i];
        std::memcpy(a, buf.data(), n * sizeof(uint32_t));
    }
}

static void radixSortInt(int* a, int n) {
    std::vector<uint32_t> pos, neg;
    pos.reserve(n); neg.reserve(n);
    for (int i = 0; i < n; ++i) {
        if (a[i] >= 0) pos.push_back((uint32_t)a[i]);
        else           neg.push_back((uint32_t)(-a[i]));
    }
    if (!neg.empty()) lsdRadix32(neg.data(), (int)neg.size());
    if (!pos.empty()) lsdRadix32(pos.data(), (int)pos.size());

    int k = 0;
    for (int i = (int)neg.size()-1; i >= 0; --i) a[k++] = -(int)neg[i];
    for (auto v : pos)                            a[k++] = (int)v;
}

static void radixSortFloat(float* a, int n) {
    std::vector<uint32_t> keys(n);
    for (int i = 0; i < n; ++i) {
        uint32_t b; std::memcpy(&b, &a[i], 4);
        keys[i] = (b >> 31) ? ~b : (b ^ 0x80000000u);
    }
    lsdRadix32(keys.data(), n);
    for (int i = 0; i < n; ++i) {
        uint32_t b = keys[i];
        b = (b >> 31) ? (b ^ 0x80000000u) : ~b;
        std::memcpy(&a[i], &b, 4);
    }
}

static void countingSortChar(signed char* a, int n) {
    int cnt[256] = {};
    for (int i = 0; i < n; ++i) cnt[(unsigned char)(a[i] + 128)]++;
    int k = 0;
    for (int v = 0; v < 256; ++v)
        for (int c = cnt[v]; c--;) a[k++] = (signed char)(v - 128);
}

static std::mt19937 RNG(0xDEADBEEF);

template<typename T> static void genRandom(T*, int);
template<> void genRandom<int>(int* a, int n) {
    std::uniform_int_distribution<int> d(INT_MIN/2, INT_MAX/2);
    for (int i = 0; i < n; ++i) a[i] = d(RNG);
}
template<> void genRandom<signed char>(signed char* a, int n) {
    std::uniform_int_distribution<int> d(-128, 127);
    for (int i = 0; i < n; ++i) a[i] = (signed char)d(RNG);
}
template<> void genRandom<float>(float* a, int n) {
    std::uniform_real_distribution<float> d(-1e6f, 1e6f);
    for (int i = 0; i < n; ++i) a[i] = d(RNG);
}

template<typename T> static void genSorted (T* a, int n) { genRandom(a,n); std::sort(a,a+n); }
template<typename T> static void genReverse(T* a, int n) { genSorted(a,n); std::reverse(a,a+n); }
template<typename T> static void genHalf   (T* a, int n) {
    genSorted(a, n/2);
    genRandom(a + n/2, n - n/2);
}
template<typename T> static void genNearly (T* a, int n) {
    genSorted(a, n);
    std::uniform_int_distribution<int> d(0, n-1);
    for (int i = 0, s = std::max(1, n/10); i < s; ++i)
        std::swap(a[d(RNG)], a[d(RNG)]);
}

template<typename T>
struct Algo {
    std::string              name;
    std::function<void(T*,int)> fn;
    bool                     quadratic; 
};

template<typename T>
static void runBenchmark(const std::string&  typeName,
                         std::ofstream&       csv,
                         std::vector<Algo<T>>& algos)
{
    using GenFn = void(*)(T*, int);
    static GenFn gens[]       = { genRandom<T>, genSorted<T>, genReverse<T>, genHalf<T>, genNearly<T> };
    static const char* labels[] = { "Random",    "Sorted",     "Reverse",     "Half",     "Nearly"     };

    struct State { double lastTimes[5]; int lastN; };
    std::vector<State> st(algos.size(), State{{0,0,0,0,0}, 0});

    for (int n : SIZES) {
        std::vector<T> base(n), arr(n);

        for (int ai = 0; ai < (int)algos.size(); ++ai) {
            auto& algo = algos[ai];
            bool isEst = algo.quadratic && n > N2_THRESHOLD && st[ai].lastN > 0;
            double times[5];

            if (isEst) {
                double ratio = (double)n / st[ai].lastN;
                for (int g = 0; g < 5; ++g)
                    times[g] = st[ai].lastTimes[g] * ratio * ratio;
            } else {
                for (int g = 0; g < 5; ++g) {
                    gens[g](base.data(), n);
                    arr = base;
                    times[g] = measure_ms([&]{ algo.fn(arr.data(), n); });
                }
                for (int g = 0; g < 5; ++g) st[ai].lastTimes[g] = times[g];
                st[ai].lastN = n;
            }

            std::cout << "[" << std::setw(10) << typeName << "]"
                      << " n=" << std::setw(10) << n
                      << "  " << algo.name
                      << (isEst ? "  [est~]" : "") << "\n";

            csv << typeName << "," << n << ",\"" << algo.name << "\"";
            for (int g = 0; g < 5; ++g)
                csv << "," << std::fixed << std::setprecision(4) << times[g];
            csv << "," << (isEst ? 1 : 0) << "\n";
        }
    }
}

int main() {
    std::ofstream csv("sorting_results.csv");
    if (!csv) { std::cerr << "Cannot open output file.\n"; return 1; }

    csv << "DataType,Size,Algorithm,Random_ms,Sorted_ms,Reverse_ms,Half_ms,Nearly_ms,Est\n";

    std::cout << "Threads : " << N_THREADS    << "\n"
              << "O(n²) cutoff : n=" << N2_THRESHOLD << " (larger sizes extrapolated)\n\n";

    {
        std::vector<Algo<int>> algos = {
            {"Insertion Sort", insertionSort<int>,                             true  },
            {"Selection Sort", selectionSort<int>,                             true  },
            {"Shell Sort",     shellSort<int>,                                 false },
            {"Quick Sort",     quickSort<int>,                                 false },
            {"Merge Sort",     mergeSort<int>,                                 false },
            {"Heap Sort",      heapSort<int>,                                  false },
            {"Tim Sort",       timSort<int>,                                   false },
            {"Radix Sort",     [](int* a,int n){ radixSortInt(a,n); },        false },
            {"std::sort",      stdSort<int>,                                   false },
            {"Parallel Sort",  parallelSort<int>,                              false },
        };
        runBenchmark<int>("int32", csv, algos);
    }

    {
        std::vector<Algo<signed char>> algos = {
            {"Insertion Sort", insertionSort<signed char>,                              true  },
            {"Selection Sort", selectionSort<signed char>,                              true  },
            {"Shell Sort",     shellSort<signed char>,                                  false },
            {"Quick Sort",     quickSort<signed char>,                                  false },
            {"Merge Sort",     mergeSort<signed char>,                                  false },
            {"Heap Sort",      heapSort<signed char>,                                   false },
            {"Tim Sort",       timSort<signed char>,                                    false },
            {"Counting Sort",  [](signed char* a,int n){ countingSortChar(a,n); },     false },
            {"std::sort",      stdSort<signed char>,                                    false },
            {"Parallel Sort",  parallelSort<signed char>,                               false },
        };
        runBenchmark<signed char>("char", csv, algos);
    }

    {
        std::vector<Algo<float>> algos = {
            {"Insertion Sort", insertionSort<float>,                               true  },
            {"Selection Sort", selectionSort<float>,                               true  },
            {"Shell Sort",     shellSort<float>,                                   false },
            {"Quick Sort",     quickSort<float>,                                   false },
            {"Merge Sort",     mergeSort<float>,                                   false },
            {"Heap Sort",      heapSort<float>,                                    false },
            {"Tim Sort",       timSort<float>,                                     false },
            {"Radix Sort",     [](float* a,int n){ radixSortFloat(a,n); },        false },
            {"std::sort",      stdSort<float>,                                     false },
            {"Parallel Sort",  parallelSort<float>,                                false },
        };
        runBenchmark<float>("float32", csv, algos);
    }

    csv.close();
    std::cout << "\nDone.  Results written to sorting_results.csv\n";
    return 0;
}
