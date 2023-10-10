#include <iostream>
#include <map>
#include <set>
#include <chrono>
#include <random>
#include <algorithm>

using namespace std;

const int MEM_SIZE = 1 << 21;
const int MIN_CACHE_SIZE = 8 * 1024;
const int MAX_CACHE_SIZE = 256 * 1024;
const int MAX_ASSOCIATIVITY = 64;
const int MIN_CACHELINE_SIZE = 16;
const int MAX_CACHELINE_SIZE = 256;
const int ACCEPTANCE_THRESHOLD = 4;
const float JUMP_THRESHOLD = 1.25;
const float JUMP_THRESHOLD_FOR_CACHE_LINE_SIZE = 1.12;
int ptr[MEM_SIZE];

// Defines some deterministic but hard-to-predict order
int fft_cmp(int a, int b) {
    if (a == b) {
        return 0;
    }
    return b & (1 << __builtin_ctz(a ^ b));
}

void generate_simple_pattern_with_stride(int spots, int raw_stride) {
    int stride = raw_stride / sizeof(int);

    vector <int> perm(spots);
    iota(perm.begin(), perm.end(), 0);
    sort(perm.begin(), perm.end(), fft_cmp);

    vector <int> inv(spots);
    for (int i = 0; i < spots; i++) {
        inv[perm[i]] = i;
    }

    for (int i = 0; i < MEM_SIZE; i++) {
        int pos_in_group = inv[(i / stride) % spots];
        if (pos_in_group + 1 < spots) {
            ptr[i] = i + stride * (perm[pos_in_group + 1] - perm[pos_in_group]);
        } else {
            int pos_in_line = i % stride;
            if (pos_in_line + 1 < stride) {
                ptr[i] = i - stride * perm[pos_in_group] + 1;
            } else {
                int no_of_group = i / (stride * spots);
                ptr[i] = (no_of_group + 1) * stride * spots;
            }
        }
    }
}

void generate_chess_pattern(int raw_stride, int raw_cachesize) {
    int stride = raw_stride / sizeof(int);
    int cachesize = raw_cachesize / sizeof(int);
    int no_of_lines = cachesize / stride;

    vector <int> perm(no_of_lines);
    iota(perm.begin(), perm.end(), 0);
    sort(perm.begin(), perm.end(), fft_cmp);

    vector <int> inv(no_of_lines);
    for (int j = 0; j < no_of_lines; j++) {
        inv[perm[j]] = j;
    }

    for (int i = 0; i < MEM_SIZE; i++) {
        int pos_in_group = inv[(i / stride) % no_of_lines];
        if ((i / cachesize) % 2 != perm[pos_in_group] % 2) {
            // i-th cell is unreachable
            continue;
        }
        if (pos_in_group + 1 < no_of_lines) {
            ptr[i] = i + stride * (perm[pos_in_group + 1] - perm[pos_in_group]);
            if (perm[pos_in_group + 1] % 2 == 1 && perm[pos_in_group] % 2 == 0) {
                ptr[i] += cachesize;
            } else if (perm[pos_in_group + 1] % 2 == 0 && perm[pos_in_group] % 2 == 1) {
                ptr[i] -= cachesize;
            }
        } else {
            int pos_in_line = i % stride;
            if (pos_in_line + 1 < stride) {
                ptr[i] = (2 * cachesize) * (i / (2 * cachesize)) + pos_in_line + 1;
            } else {
                ptr[i] = (2 * cachesize) * (i / (2 * cachesize) + 1);
            }
        }
    }
}

long long walk_and_measure_time() {
    const auto start = chrono::high_resolution_clock::now(); 
    for (int pos = 0; pos < MEM_SIZE; pos = ptr[pos]);
    const auto end = chrono::high_resolution_clock::now();

    return chrono::duration_cast<chrono::nanoseconds>(end - start).count();
}

pair <int, int> find_cachesize_and_associativity() {
    map <int, int> number_of_detections;
    map <int, set <int>> spots_at_detections;
    map <int, int> possible_associativity;

    for (int stride = MIN_CACHELINE_SIZE; stride < MAX_CACHE_SIZE; stride *= 2) {
        float avg_time_prev = -1;   
        for (int spots = 2; spots * stride / sizeof(int) <= MEM_SIZE; spots *= 2) {
            int expected_cache_size = (spots / 2) * stride;
            if (expected_cache_size < MIN_CACHE_SIZE) {
                continue;
            }
            if (expected_cache_size > 2 * MAX_CACHE_SIZE) {
                break;
            }

            generate_simple_pattern_with_stride(spots, stride);
            float avg_time = 1.0 * walk_and_measure_time() / MEM_SIZE;
            float ratio = (avg_time_prev == -1) ? 0 : avg_time / avg_time_prev;
            avg_time_prev = avg_time;

            if (ratio > JUMP_THRESHOLD) {
                //cout << "Find possible jump, detected size is " << expected_cache_size << " spot is " << spots / 2 << " ratio is " << ratio << endl;
                number_of_detections[expected_cache_size]++;
                spots_at_detections[expected_cache_size].insert(spots / 2);
                if (spots / 2 < MAX_ASSOCIATIVITY && spots_at_detections[expected_cache_size / 2].find(spots / 2) != spots_at_detections[expected_cache_size / 2].end()) {
                    possible_associativity[expected_cache_size / 2] = spots / 2;
                }
            }
        }
    }

    int associativity = -1;
    int cachesize = -1;
    for (auto [size, cnt] : spots_at_detections) {
        if (cnt.size() >= ACCEPTANCE_THRESHOLD) {
            associativity = possible_associativity[size];
            cachesize = size;
            break;
        }
    }

    return {cachesize, associativity};
}

int find_cacheline_size(int cachesize) {
    float avg_time_prev = -1;
    float max_ratio = 0;
    int stride_at_max_ratio = -1;
    for (int stride = MIN_CACHELINE_SIZE / 2; stride <= MAX_CACHELINE_SIZE; stride *= 2) {
        generate_chess_pattern(stride, cachesize);
        long double avg_time = 2.0L * walk_and_measure_time() / MEM_SIZE;
        long double ratio = (avg_time_prev == -1) ? 0 : avg_time_prev / avg_time;
        avg_time_prev = avg_time;

        //cout << stride << ' ' << avg_time << ' ' << ratio << endl;
        if (ratio > JUMP_THRESHOLD_FOR_CACHE_LINE_SIZE) {
            return stride;
        }
    }
    return -1;
}

int main() {
    //posix_memalign((void**) &ptr, sysconf(_SC_PAGESIZE), MEM_SIZE * sizeof(int));
    auto [cachesize, associativity] = find_cachesize_and_associativity();

    if (cachesize == -1) {
        cout << "Failed to determine cache characteristics. Please, try again" << endl;
        return 1;
    }

    cout << "Cache size = " << cachesize << endl;
    cout << "Associativity = " << associativity << endl;

    int cacheline_size = find_cacheline_size(cachesize);
    
    if (cacheline_size == -1) {
        cout << "Failed to determine cache line size. Please, try again" << endl;
        return 1;
    } else {
        cout << "Cache line size = " << cacheline_size << endl;
    }
    return 0;
}
