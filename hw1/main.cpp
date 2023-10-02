#include <iostream>
#include <map>
#include <set>
#include <chrono>
#include <random>
#include <algorithm>

using namespace std;

const int MEM_SIZE = 1 << 25;
const int MIN_CACHE_SIZE = 4 * 1024;
const int MAX_CACHE_SIZE = 256 * 1024;
const int MAX_ASSOCIATIVITY = 64;
const int ACCEPTANCE_THRESHOLD = 4;
const long double JUMP_THRESHOLD = 1.15;
int ptr[MEM_SIZE];

mt19937 rnd(239);

void fill_with_stride(int stride) {
    for (int i = 0; i < MEM_SIZE; i++) {
        ptr[i] = max(0, i - stride);
    }
}

void fill_with_k_parallel_lines(int spots, int raw_stride) {
    int stride = raw_stride / sizeof(int);
    vector <int> perm(spots);
    iota(perm.begin(), perm.end(), 0);
    shuffle(perm.begin() + 1, perm.end(), rnd);
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
            if (pos_in_line != stride - 1) {
                ptr[i] = i - stride * perm[no_in_group] + 1;
            } else {
                int no_of_block = i / (stride * spots);
                ptr[i] = (no_of_block + 1) * stride * spots;
            }
        }
    }
}

void walk_all() {
    // int cnt = 0;
    for (int pos = 0; pos < MEM_SIZE; pos = ptr[pos]);
    //cout << cnt << endl;
    //exit(0);
}

int main() {
    //posix_memalign((void**) &ptr, sysconf(_SC_PAGESIZE), MEM_SIZE * sizeof(int));
    map <int, int> number_of_detections;
    map <int, set <int>> spots_at_detections;
    map <int, int> possible_associativity;
    map <int, int> stride_at_first_detection;
    for (int stride = 32; stride <= (1 << 18); stride *= 2) {
        long double avg_time_prev = -1;
        for (int spots = 2; spots * stride / sizeof(int) <= MEM_SIZE; spots *= 2) {
            int expected_cache_size_if_jump = (spots / 2) * stride;
            if (expected_cache_size_if_jump < MIN_CACHE_SIZE) {
                continue;
            }
            if (expected_cache_size_if_jump > 2 * MAX_CACHE_SIZE) {
                break;
            }

            //cout << "Currently processing stride = " << stride << "; spots = " << spots << endl;
            fill_with_k_parallel_lines(spots, stride);

            const auto start = chrono::high_resolution_clock::now();
            walk_all();
            const auto end = chrono::high_resolution_clock::now();
            
            const auto duration = end - start;
            auto nanoseconds = chrono::duration_cast<chrono::nanoseconds>(duration).count();
            const long double avg_time = 1.0L * nanoseconds / MEM_SIZE;
            long double ratio = (avg_time_prev == -1) ? 0 : avg_time / avg_time_prev;
            // cout << "stride = " << stride << "; number of spots = " << spots << "; avg time = " << avg_time << "; ratio = " << ratio << "\n";
            avg_time_prev = avg_time;
            if (ratio > JUMP_THRESHOLD) {
                cout << "Find possible jump, detected size is " << expected_cache_size_if_jump << " spot is " << spots / 2 << " ratio is " << ratio << endl;
                number_of_detections[expected_cache_size_if_jump]++;
                spots_at_detections[expected_cache_size_if_jump].insert(spots / 2);
                if (stride_at_first_detection.find(expected_cache_size_if_jump) == stride_at_first_detection.end()) {
                    stride_at_first_detection[expected_cache_size_if_jump] = stride;
                }
                if (spots / 2 < MAX_ASSOCIATIVITY && spots_at_detections[expected_cache_size_if_jump / 2].find(spots / 2) != spots_at_detections[expected_cache_size_if_jump / 2].end() && spots_at_detections[expected_cache_size_if_jump / 2].find(spots / 4) == spots_at_detections[expected_cache_size_if_jump / 2].end()) {
                    possible_associativity[expected_cache_size_if_jump / 2] = spots / 2;
                }
            }
        }
    }
    for (auto [size, cnt] : spots_at_detections) {
        cout << size << ' ' << number_of_detections[size] << ' ' << stride_at_first_detection[size] << endl;
        cout << possible_associativity[size] << endl;
        cout << "======" << endl;
        // if (cnt.size() >= ACCEPTANCE_THRESHOLD) {
        //     int associativity = possible_associativity[size];
        //     int cacheline_size = stride_at_first_detection[size];
        //     cout << "Cache size = " << size << endl;
        //     cout << "Cache associativity = " << associativity << endl;
        //     cout << "Cacheline size = " << cacheline_size << endl;
        //     return 0;
        // }
    }
    cout << "Failed to determine cache characteristics. Please, try again" << endl;
    return 1;
}
