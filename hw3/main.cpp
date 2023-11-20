#include <string.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iostream>

extern "C" {
    #include "byterun.h"
}

struct code_instruction {
    const char *ptr;
    int len;

    code_instruction() {}
    code_instruction(const char *ptr, int len): ptr(ptr), len(len) {}

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

void eval_counter(FILE *f, bytefile *bf) {
    std::unordered_map<code_instruction, int> counter;
    const char *ip = bf->code_ptr;

    while (ip < bf->code_ptr + bf->bytecode_size) {
        const char *new_ip = disassemble_one_instruction(nullptr, bf, ip);
        ++counter[code_instruction(ip, new_ip - ip)];
        ip = new_ip;
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
        const char *ip = code_instruction.ptr;
        fprintf(f, "%d times: \"", cnt);
        disassemble_one_instruction(f, bf, ip);
        fprintf(f, "\"\n");
    }
}

int main(int argc, char* argv[]) {
    bytefile *f = read_file (argv[1]);
    eval_counter(stdout, f);
    return 0;
}