#include <string.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

extern "C" {
    #include "byterun.h"
}

struct code_instruction {
    char *ptr;
    int len;

    code_instruction() {}
    code_instruction(char *ptr, int len): ptr(ptr), len(len) {}

    bool operator == (const code_instruction &other) const {
        return len == other.len && !memcmp(ptr, other.ptr, len);
    }
};

template <>
struct std::hash<code_instruction> {
    std::size_t operator () (const code_instruction &instr) const {
        std::size_t result = 0;
        for (int i = 0; i < instr.len; i++) {
            result = (result << 8) + instr.ptr[i];
        }
        return result;
    }
};

void eval_counter(FILE *f, FILE *dump, bytefile *bf) {
    std::unordered_map<code_instruction, int> counter;
    char *ip = bf->code_ptr;
    char *old_value = ip;

    while (disassemble_one_instruction(dump, bf, &ip)) {
        ++counter[code_instruction(old_value, ip - old_value)];
        old_value = ip;
    }

    std::vector<std::pair<code_instruction, int>> counter_sorted;
    for (auto const &elem : counter) {
        counter_sorted.push_back(elem);
    }
    std::sort(counter_sorted.begin(), counter_sorted.end(), [](auto const &p1, auto const &p2) {
        return p1.second > p2.second;
    });

    fprintf(f, "Results of frequency analysis:\n");
    for (auto const &[code_instruction, cnt] : counter_sorted) {
        char *ip = code_instruction.ptr;
        fprintf(f, "%d times: \"", cnt);
        disassemble_one_instruction(f, bf, &ip);
        fprintf(f, "\"\n");
    }
}

int main(int argc, char* argv[]) {
    bytefile *f = read_file (argv[1]);
    FILE *dump = fopen("/dev/null", "w");
    eval_counter(stdout, dump, f);
    return 0;
}