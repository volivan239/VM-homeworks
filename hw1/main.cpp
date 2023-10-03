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
const int ACCEPTANCE_THRESHOLD = 4;
const long double JUMP_THRESHOLD = 1.2;
const long double JUMP_THRESHOLD_FOR_CACHE_LINE_SIZE = 1.12;
int ptr[MEM_SIZE];

mt19937 rnd(239);

int fft_cmp(int a, int b) {
    return b & (1 << __builtin_ctz(a ^ b));
}

void fill_with_k_parallel_lines(int spots, int raw_stride) {
    int stride = raw_stride / sizeof(int);
    vector <int> perm(spots);
    iota(perm.begin(), perm.end(), 0);
    sort(perm.begin() + 1, perm.end(), fft_cmp);
    vector <int> inv(spots);
    for (int i = 0; i < spots; i++) {
        inv[perm[i]] = i;
    }
    for (int i = 0; i < MEM_SIZE; i++) {
        int no_in_group = inv[(i / stride) % spots];
        if (no_in_group + 1 < spots) {
            ptr[i] = i + stride * (perm[no_in_group + 1] - perm[no_in_group]);
        } else {
            int pos_in_line = i % stride;
            if (pos_in_line + 1 < stride) {
                ptr[i] = i - stride * perm[no_in_group] + 1;
            } else {
                int no_of_block = i / (stride * spots);
                ptr[i] = (no_of_block + 1) * stride * spots;
            }
        }
    }
}

void fill_chess_like(int raw_stride, int raw_cachesize) {
    int stride = raw_stride / sizeof(int);
    int cachesize = raw_cachesize / sizeof(int);
    int no_of_lines = cachesize / stride;
    vector <int> perm(no_of_lines);
    vector <int> inv(no_of_lines);
    iota(perm.begin(), perm.end(), 0);
    sort(perm.begin(), perm.end(), fft_cmp);

    for (int i = 0; i < MEM_SIZE; i++) {
        if (i % (2 * cachesize) == 0) {
            //shuffle(perm.begin() + 1, perm.end(), rnd);
            for (int j = 0; j < no_of_lines; j++) {
                inv[perm[j]] = j;
            }
        }
        int no_in_group = inv[(i / stride) % no_of_lines];
        if ((i / cachesize) % 2 != perm[no_in_group] % 2) {
            // i-th cell is unreachable
            continue;
        }
        if (no_in_group + 1 < no_of_lines) {
            ptr[i] = i + stride * (perm[no_in_group + 1] - perm[no_in_group]);
            if (perm[no_in_group + 1] % 2 == 1 && perm[no_in_group] % 2 == 0) {
                ptr[i] += cachesize;
            } else if (perm[no_in_group + 1] % 2 == 0 && perm[no_in_group] % 2 == 1) {
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
    int cnt = 0;
    for (int pos = 0; pos < MEM_SIZE; pos = ptr[pos]);
    const auto end = chrono::high_resolution_clock::now();

    return chrono::duration_cast<chrono::nanoseconds>(end - start).count();
}

int main() {
    //posix_memalign((void**) &ptr, sysconf(_SC_PAGESIZE), MEM_SIZE * sizeof(int));
    map <int, int> number_of_detections;
    map <int, set <int>> spots_at_detections;
    map <int, int> possible_associativity;
    for (int stride = MIN_CACHELINE_SIZE; stride < MAX_CACHE_SIZE; stride *= 2) {
        long double avg_time_prev = -1;
        for (int spots = 2; spots * stride / sizeof(int) <= MEM_SIZE; spots *= 2) {
            int expected_cache_size_if_jump = (spots / 2) * stride;
            if (expected_cache_size_if_jump < MIN_CACHE_SIZE) {
                continue;
            }
            if (expected_cache_size_if_jump > 2 * MAX_CACHE_SIZE) {
                break;
            }

            fill_with_k_parallel_lines(spots, stride);
            long double avg_time = 1.0L * walk_and_measure_time() / MEM_SIZE;
            long double ratio = (avg_time_prev == -1) ? 0 : avg_time / avg_time_prev;
            avg_time_prev = avg_time;

            if (ratio > JUMP_THRESHOLD) {
                cout << "Find possible jump, detected size is " << expected_cache_size_if_jump << " spot is " << spots / 2 << " ratio is " << ratio << endl;
                number_of_detections[expected_cache_size_if_jump]++;
                spots_at_detections[expected_cache_size_if_jump].insert(spots / 2);
                if (spots / 2 < MAX_ASSOCIATIVITY && spots_at_detections[expected_cache_size_if_jump / 2].find(spots / 2) != spots_at_detections[expected_cache_size_if_jump / 2].end()) {// && spots_at_detections[expected_cache_size_if_jump / 2].find(spots / 4) == spots_at_detections[expected_cache_size_if_jump / 2].end()) {
                    possible_associativity[expected_cache_size_if_jump / 2] = spots / 2;
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
    if (cachesize == -1) {
        cout << "Failed to determine cache characteristics. Please, try again" << endl;
        return 1;
    }
    cout << "Cache size = " << cachesize << endl;
    cout << "Associativity = " << associativity << endl;
    long double avg_time_prev = -1;
    for (int stride = MIN_CACHELINE_SIZE; stride < MAX_CACHE_SIZE; stride *= 2) {
        fill_chess_like(stride, cachesize);
        long double avg_time = 2.0L * walk_and_measure_time() / MEM_SIZE;
        long double ratio = (avg_time_prev == -1) ? 0 : avg_time_prev / avg_time;
        avg_time_prev = avg_time;

        cout << stride << ' ' << avg_time << ' ' << ratio << endl;
        if (ratio > JUMP_THRESHOLD_FOR_CACHE_LINE_SIZE) {
            cout << "Cache line size = " << stride << endl;
            return 0;
        }
    }
    cout << "Failed to determine cache line size. Please, try again" << endl;
    return 0;
}
