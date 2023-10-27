#!/usr/bin/python3
import os
import subprocess

base_test_dir = '../../Lama/regression/'
test_dirs = ['.', 'expressions', 'deep-expressions']
lama_compiler = 'lamac'
logs_dir = './logs'
tests_total = 0
tests_success = 0

if not os.path.exists(logs_dir):
    os.makedirs(logs_dir)

for test_dir in test_dirs:
    cur_test_dir = os.path.join(base_test_dir, test_dir)
    basic_tests = sorted([os.path.splitext(f)[0] for f in os.listdir(cur_test_dir) if f.endswith('.lama')])


    for test in basic_tests:
        print(f'Evaluating {test_dir}/{test}.lama:')
        tests_total += 1

        src_file = os.path.join(cur_test_dir, test + '.lama')
        binary_file = test + '.bc'
        input_file = os.path.join(cur_test_dir, test + '.input')
        expected_file = os.path.join(cur_test_dir, 'orig', test + '.log')
        actual_file = os.path.join(logs_dir, test + '.log')

        subprocess.run([lama_compiler, '-b', src_file])
        
        with open(input_file, 'r') as inf:
            with open(actual_file, 'w') as ouf:
                result = subprocess.run(['./build/interpreter', binary_file], stdin=inf, stdout=ouf)

        if result.returncode != 0:
            print(f'ERROR! Interpreter returned {result.returncode}')
            exit(-1)

        if subprocess.run(['diff', expected_file, actual_file]).returncode != 0:
            print('ERROR! Output differs from expected')
            exit(-1)

        tests_success += 1
        print('OK')

print(f'Total tests: {tests_total}, successful: {tests_success}')
if tests_success < tests_total:
    exit(1)
