#include "callstack.h"
#include <iostream>

extern "C" {
  #include "runtime.h"
  extern void* Belem_link (void *p, int i);
}

const int MAX_STACK_SIZE = 1024 * 1024;

callstack::callstack(int32_t *&stack_top, int32_t *&stack_bottom): stack_top(stack_top), stack_bottom(stack_bottom) {    
  fp = stack_bottom = stack_top = (new int[MAX_STACK_SIZE]) + MAX_STACK_SIZE;
  push(0); // fake argv
  push(0); // fake argc
  push(2);
};

callstack::~callstack() {
  delete[] (stack_top - MAX_STACK_SIZE);
}

int32_t* callstack::get_stack_bottom() {
  return stack_bottom;
}

int32_t box(int32_t value) {
  return (value << 1) | 1;
}

int32_t unbox(int32_t value) {
  return value >> 1;
}

bool is_boxed(int32_t value) {
  return value & 1;
}

void callstack::push(int32_t value) {
  *(--stack_bottom) = value;
}

int32_t callstack::pop_unchecked() {
  return *(stack_bottom++);
}

int32_t callstack::pop() {
  assert(stack_bottom < fp);
  return pop_unchecked();
}

int32_t callstack::nth(int n) {
  return stack_bottom[n];
}

void callstack::drop(int n) {
  stack_bottom += n;
}

void callstack::fill(int n, int32_t value) {
  for (int i = 0; i < n; i++) {
    push(value);
  }
}

void callstack::reverse(int n) {
  int32_t *st = stack_bottom; // Point to last element
  int32_t *first_arg = st + n - 1; // Points to first argument
  while (st < first_arg) {
    std::swap(*(st++), *(first_arg--));
  }
}

void callstack::prologue(int32_t nlocals, int32_t nargs) {
  push(reinterpret_cast<int32_t>(fp));
  fp = stack_bottom;
  fill(nlocals, box(0));
}

char* callstack::epilogue() {
  int32_t rv = pop();
  stack_bottom = fp;
  fp = reinterpret_cast<int32_t*>(pop_unchecked());
  int32_t nargs = pop();
  char *ra = reinterpret_cast<char*>(pop());
  drop(nargs);
  push(rv);
  return ra;
}

int32_t* callstack::get_current_closure() {
  int32_t nargs = *(fp + 1);
  return reinterpret_cast<int32_t*>(*arg(nargs - 1));
}

int32_t* callstack::local(int pos) {
  return fp - pos - 1;
}

int32_t* callstack::arg(int pos) {
  return fp + pos + 3;
}

int32_t* callstack::closure_binded(int pos) {
  int32_t *closure = get_current_closure();
  return reinterpret_cast<int32_t*>(Belem_link(closure, box(pos + 1)));
}

void callstack::binop(int opcode) {
  int32_t rhv = unbox(pop());
  int32_t lhv = unbox(pop());
  int32_t result;
  switch (opcode) {
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