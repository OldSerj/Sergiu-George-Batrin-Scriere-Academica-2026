#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>


// ======================================================
// HIGH‑PRECISION TIMER
// ======================================================

double measure(void (*f)(int[], int), int arr[], int n) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    f(arr, n);

    QueryPerformanceCounter(&end);
    return (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
}

#else
#define _POSIX_C_SOURCE 199309L
#include <time.h>

double measure(void (*f)(int[], int), int arr[], int n) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    f(arr, n);
    clock_gettime(CLOCK_MONOTONIC, &end);

    return (end.tv_sec - start.tv_sec)
         + (end.tv_nsec - start.tv_nsec) / 1e9;
}
#endif

// ======================================================
// SORTING ALGORITHMS
// ======================================================

// Insertion Sort
void insertionSort(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// Selection Sort
void selectionSort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int minIndex = i;
        for (int j = i + 1; j < n; j++)
            if (arr[j] < arr[minIndex])
                minIndex = j;

        int tmp = arr[i];
        arr[i] = arr[minIndex];
        arr[minIndex] = tmp;
    }
}

// Quick Sort
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            int t = arr[i];
            arr[i] = arr[j];
            arr[j] = t;
        }
    }

    int t = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = t;

    return i + 1;
}

void quickSortRec(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSortRec(arr, low, pi - 1);
        quickSortRec(arr, pi + 1, high);
    }
}

void quickSortWrapper(int arr[], int n) {
    quickSortRec(arr, 0, n - 1);
}

// Merge Sort
void merge(int arr[], int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;

    while (i < n1 && j < n2)
        arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];

    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}

void mergeSortRec(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortRec(arr, l, m);
        mergeSortRec(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

void mergeSortWrapper(int arr[], int n) {
    mergeSortRec(arr, 0, n - 1);
}

// Heap Sort
void heapify(int arr[], int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && arr[left] > arr[largest]) largest = left;
    if (right < n && arr[right] > arr[largest]) largest = right;

    if (largest != i) {
        int t = arr[i];
        arr[i] = arr[largest];
        arr[largest] = t;
        heapify(arr, n, largest);
    }
}

void heapSort(int arr[], int n) {
    for (int i = n / 2 - 1; i >= 0; i--)
        heapify(arr, n, i);

    for (int i = n - 1; i > 0; i--) {
        int t = arr[0];
        arr[0] = arr[i];
        arr[i] = t;
        heapify(arr, i, 0);
    }
}

// Radix Sort
int max_element(int arr[], int n) {
    int maxVal = arr[0];
    for (int i = 1; i < n; i++)
        if (arr[i] > maxVal)
            maxVal = arr[i];
    return maxVal;
}

void countingSort(int arr[], int n, int exp) {
    int *output = (int *)malloc(n * sizeof(int));
    int count[10] = {0};

    for (int i = 0; i < n; i++)
        count[(arr[i] / exp) % 10]++;

    for (int i = 1; i < 10; i++)
        count[i] += count[i - 1];

    for (int i = n - 1; i >= 0; i--) {
        int digit = (arr[i] / exp) % 10;
        output[count[digit] - 1] = arr[i];
        count[digit]--;
    }

    for (int i = 0; i < n; i++)
        arr[i] = output[i];

    free(output);
}

void radixSort(int arr[], int n) {
    int maxVal = max_element(arr, n);
    for (int exp = 1; maxVal / exp > 0; exp *= 10)
        countingSort(arr, n, exp);
}

// Tim Sort (simplified)
void timSort(int arr[], int n) {
    const int RUN = 32;

    for (int i = 0; i < n; i += RUN) {
        int len = (i + RUN < n) ? RUN : (n - i);
        insertionSort(arr + i, len);
    }

    for (int size = RUN; size < n; size *= 2) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = (left + 2 * size - 1 < n - 1)
                        ? left + 2 * size - 1
                        : n - 1;

            if (mid < right)
                merge(arr, left, mid, right);
        }
    }
}

// ======================================================
// ARRAY GENERATORS
// ======================================================

void generateRandom(int arr[], int n) {
    for (int i = 0; i < n; i++)
        arr[i] = rand() % 1000000;
}

void generateSorted(int arr[], int n) {
    for (int i = 0; i < n; i++)
        arr[i] = i;
}

void generateReverse(int arr[], int n) {
    for (int i = 0; i < n; i++)
        arr[i] = n - i;
}

void generateHalf(int arr[], int n) {
    for (int i = 0; i < n / 2; i++)
        arr[i] = i;
    for (int i = n / 2; i < n; i++)
        arr[i] = rand() % 1000000;
}

void generateNearly(int arr[], int n) {
    generateSorted(arr, n);
    for (int i = 0; i < n / 10; i++) {
        int a = rand() % n;
        int b = rand() % n;
        int t = arr[a];
        arr[a] = arr[b];
        arr[b] = t;
    }
}

// ======================================================
// MAIN BENCHMARK
// ======================================================

int main() {
    srand(time(NULL));

    int sizes[] = {20, 50, 100, 500, 1000, 5000, 10000};
    int numSizes = sizeof(sizes) / sizeof(sizes[0]);

    FILE *out = fopen("sorting_results.csv", "w");
    fprintf(out, "Size,Algorithm,Random,Sorted,Reverse,Half,Nearly\n");

    struct {
        const char *name;
        void (*func)(int[], int);
    } algos[] = {
        {"Insertion Sort", insertionSort},
        {"Selection Sort", selectionSort},
        {"Quick Sort",    quickSortWrapper},
        {"Merge Sort",    mergeSortWrapper},
        {"Heap Sort",     heapSort},
        {"Radix Sort",    radixSort},
        {"Tim Sort",      timSort}
    };

    int numAlgos = sizeof(algos) / sizeof(algos[0]);

    for (int s = 0; s < numSizes; s++) {
        int n = sizes[s];

        int *base = (int *)malloc(n * sizeof(int));
        int *arr  = (int *)malloc(n * sizeof(int));

        for (int a = 0; a < numAlgos; a++) {
            double t[5];

            void (*gens[5])(int[], int) = {
                generateRandom,
                generateSorted,
                generateReverse,
                generateHalf,
                generateNearly
            };

            const char *types[5] = {
                "Random", "Sorted", "Reverse", "Half", "Nearly"
            };

            for (int g = 0; g < 5; g++) {
                printf("[SIZE %d] Running %s on %s...\n",
                       n, algos[a].name, types[g]);

                gens[g](base, n);
                memcpy(arr, base, n * sizeof(int));

                t[g] = measure(algos[a].func, arr, n);

                printf("[SIZE %d] %s on %s completed in %.6f s\n",
                       n, algos[a].name, types[g], t[g]);
            }

            fprintf(out, "%d,%s,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                    n, algos[a].name,
                    t[0], t[1], t[2], t[3], t[4]);
        }

        free(base);
        free(arr);
    }

    fclose(out);
    return 0;
}
