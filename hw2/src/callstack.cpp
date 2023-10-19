#include "callstack.h"
#include <iostream>

extern "C" {
  #include "runtime.h"
}

callstack::callstack(int32_t *&stack_top, int32_t *&stack_bottom): stack_top(stack_top), stack_bottom(stack_bottom) {    
  stack_bottom = stack_top = (new int[1024]) + 1024;
  push(0);
};

int32_t box(int32_t value) {
  return (value << 1) | 1;
}

int32_t unbox(int32_t value) {
  return value >> 1;
}

void callstack::push(int32_t value) {
  *(--stack_bottom) = value;
}

int32_t callstack::pop() {
  return *(stack_bottom++);
}

void callstack::drop(int n) {
  stack_bottom += n;
}

void callstack::reserve(int n) {
  for (int i = 0; i < n; i++) {
    push(box(0));
  }
}

void callstack::prologue(int32_t nlocals, int32_t nargs) {
  push(nargs);
  push(reinterpret_cast<int32_t>(fp));
  fp = stack_bottom;
  reserve(nlocals);
}

char* callstack::epilogue() {
  // std::cerr << stack_bottom << ' ' << stack_top << std::endl;
  int32_t rv = pop();
  stack_bottom = fp;
  fp = reinterpret_cast<int32_t*>(pop());
  int32_t nargs = pop();
  char *ra = reinterpret_cast<char*>(pop());
  drop(nargs);
  push(rv);
  //std::cerr << rv << ' ' << ra << std::endl;
  return ra;
}

int32_t& callstack::local(int pos) {
  return fp[-pos - 1];
}

int32_t& callstack::arg(int pos) {
  return fp[pos + 3];
}

void callstack::binop(int opcode) {
  int32_t rhv = unbox(pop());
  int32_t lhv = unbox(pop());
  int32_t result;
  switch(opcode) {
    case 1:
      result = lhv + rhv;
      break;
    case 2:
      result = lhv - rhv;
      break;
    case 3:
      result = lhv * rhv;
      break;
    case 4:
      result = lhv / rhv;
      break;
    case 5:
      result = lhv % rhv;
      break;
    case 6:
      result = lhv < rhv;
      break;
    case 7:
      result = lhv <= rhv;
      break;
    case 8:
      result = lhv > rhv;
      break;
    case 9:
      result = lhv >= rhv;
      break;
    case 10:
      result = lhv == rhv;
      break;
    case 11:
      result = lhv != rhv;
      break;
    case 12:
      result = lhv && rhv;
      break;
    case 13:
      result = lhv || rhv;
      break;
    default:
      failure("Unexpected binary operation code: %d", opcode);
  }
  push(box(result));
}