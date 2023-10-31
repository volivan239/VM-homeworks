#include "interpreter.h"
#include <iostream>

extern "C" {
  extern void *alloc (size_t size);
  extern int Lread();
  extern int Lwrite(int);
  extern int Llength(void*);
  extern void* Lstring (void *p);
  extern void* Bstring(void*);
  extern void* Belem (void *p, int i);
  extern void* Bsta (void *v, int i, void *x);
  extern void* Barray_my (int bn, int *data_);
  extern void* Bsexp_my (int bn, int tag, int *data_);
  extern int LtagHash (char *s);
  extern int Btag (void *d, int t, int n);
  extern int Barray_patt (void *d, int n);
  extern void* Bclosure_my (int bn, void *entry, int *values);
  extern void* Belem_link (void *p, int i);
  extern int Bstring_patt (void *x, void *y);
  extern int Bstring_tag_patt (void *x);
  extern int Barray_tag_patt (void *x);
  extern int Bsexp_tag_patt (void *x);
  extern int Bunboxed_patt (void *x);
  extern int Bboxed_patt (void *x);
  extern int Bclosure_tag_patt (void *x);
}

const int MAX_STACK_SIZE = 1024 * 1024;

interpreter::interpreter(bytefile *bf, int32_t *&stack_top, int32_t *&stack_bottom):
  bf(bf), stack_top(stack_top), stack_bottom(stack_bottom), ip(bf->code_ptr) {

  fp = stack_bottom = stack_top = (new int[MAX_STACK_SIZE]) + MAX_STACK_SIZE;
  push(0); // fake argv
  push(0); // fake argc
  push(2);
}


int32_t* interpreter::get_stack_bottom() {
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

void interpreter::push(int32_t value) {
  if (stack_bottom == stack_top - MAX_STACK_SIZE) {
    failure("Stack limit exceeded");
  }
  *(--stack_bottom) = value;
}

int32_t interpreter::pop_unchecked() {
  return *(stack_bottom++);
}

int32_t interpreter::pop() {
  if (stack_bottom >= fp) {
    failure("Illegal pop occured");
  }
  return pop_unchecked();
}

int32_t interpreter::nth(int n) {
  return stack_bottom[n];
}

void interpreter::drop(int n) {
  stack_bottom += n;
}

void interpreter::fill(int n, int32_t value) {
  for (int i = 0; i < n; i++) {
    push(value);
  }
}

void interpreter::reverse(int n) {
  int32_t *st = stack_bottom; // Point to last element
  int32_t *first_arg = st + n - 1; // Points to first argument
  while (st < first_arg) {
    std::swap(*(st++), *(first_arg--));
  }
}

void interpreter::prologue(int32_t nlocals, int32_t nargs) {
  push(reinterpret_cast<int32_t>(fp));
  fp = stack_bottom;
  fill(nlocals, box(0));
}

char* interpreter::epilogue() {
  int32_t rv = pop();
  stack_bottom = fp;
  fp = reinterpret_cast<int32_t*>(pop_unchecked());
  int32_t nargs = pop();
  char *ra = reinterpret_cast<char*>(pop());
  drop(nargs);
  push(rv);
  return ra;
}

int32_t* interpreter::get_current_closure() {
  int32_t nargs = *(fp + 1);
  return reinterpret_cast<int32_t*>(*arg(nargs - 1));
}

int32_t* interpreter::local(int pos) {
  return fp - pos - 1;
}

int32_t* interpreter::arg(int pos) {
  return fp + pos + 3;
}

int32_t* interpreter::closure_binded(int pos) {
  int32_t *closure = get_current_closure();
  return reinterpret_cast<int32_t*>(Belem_link(closure, box(pos + 1)));
}

interpreter::~interpreter() {
  delete[] (stack_top - MAX_STACK_SIZE);
}

int32_t interpreter::next_int() {
  int32_t result = *reinterpret_cast<int32_t*>(ip);
  ip += 4;
  return result;
}

char interpreter::next_char() {
  return *ip++;
}

char* interpreter::next_str() {
  int32_t pos = next_int();
  return bf->get_string(pos);
}

int32_t* interpreter::global(int32_t pos) {
  return bf->global_ptr + pos;
}

int32_t* interpreter::get_by_location(char l, int32_t value) {
  switch (l) {
    case 0: return global(value);
    case 1: return local(value);
    case 2: return arg(value);
    case 3: return closure_binded(value);
    default: inst_decode_failure();
  }
}

void interpreter::inst_decode_failure() {
  failure("ERROR: invalide opcode");
}

void interpreter::eval_binop(char l) {
  int32_t rhv = unbox(pop());
  int32_t lhv = unbox(pop());
  int32_t result;
  switch (l) {
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
      failure("Unexpected binary operation code: %d", l);
  }
  push(box(result));
}

void interpreter::eval_const() {
  push(box(next_int()));
}

void interpreter::eval_end() {
  ip = epilogue();
}

void interpreter::eval_drop() {
  pop();
}

void interpreter::eval_st(char l) {
  int32_t ind = next_int();
  int32_t value = pop();
  *get_by_location(l, ind) = value;
  push(value);
}

void interpreter::eval_ld(char l) {
  int32_t ind = next_int();
  int32_t value = *get_by_location(l, ind);
  push(value);
}

void interpreter::eval_begin() {
  int nargs = next_int();
  int nlocals = next_int();
  prologue(nlocals, nargs);
}

void interpreter::eval_cbegin() {
  eval_begin();
}

void interpreter::eval_read() {
  push(Lread());
}

void interpreter::eval_write() {
  push(Lwrite(pop()));
}

void interpreter::eval_line() {
  next_int();
}

void interpreter::eval_jmp() {
  ip = bf->code_ptr + next_int();
}

void interpreter::eval_cjmp_nz() {
  int32_t shift = next_int();
  if (unbox(pop()) != 0) {
    ip = bf->code_ptr + shift;
  }
}

void interpreter::eval_cjmp_z() {
  int32_t shift = next_int();
  if (unbox(pop()) == 0) {
    ip = bf->code_ptr + shift;
  }
}

void interpreter::eval_call() {
  int32_t shift = next_int();
  int32_t nargs = next_int();
  reverse(nargs);
  push(reinterpret_cast<int32_t>(ip));
  push(nargs);
  ip = bf->code_ptr + shift;
}

void interpreter::eval_callc() {
  int32_t nargs = next_int();
  char* callee = reinterpret_cast<char*>(Belem(reinterpret_cast<int32_t*>(nth(nargs)), box(0)));
  reverse(nargs);
  push(reinterpret_cast<int32_t>(ip));
  push(nargs + 1);
  ip = callee;
}

void interpreter::eval_string() {
  push(reinterpret_cast<int32_t>(Bstring(bf->string_ptr + next_int())));
}

void interpreter::eval_length() {
  push(Llength(reinterpret_cast<void*>(pop())));
}

void interpreter::eval_sta() {
  void *v = reinterpret_cast<void*>(pop());
  int32_t i = pop();
  if (is_boxed(i)) {   
    void *x = reinterpret_cast<void*>(pop());
    push(reinterpret_cast<int32_t>(Bsta(v, i, x))); 
  } else {
    push(reinterpret_cast<int32_t>(Bsta(v, i, nullptr)));
  }
}

void interpreter::eval_elem() {
  int32_t v = pop();
  void *p = reinterpret_cast<void*>(pop());
  push(reinterpret_cast<int32_t>(Belem(p, v)));
}

void interpreter::eval_barray() {
  int32_t len = next_int();
  reverse(len);
  int32_t res = reinterpret_cast<int32_t>(Barray_my(box(len), get_stack_bottom()));
  drop(len);
  push(res);
}

void interpreter::eval_sexp() {
  char *name = next_str();
  int32_t len = next_int();
  int32_t tag = LtagHash(name);
  reverse(len);
  int32_t res = reinterpret_cast<int32_t>(Bsexp_my(box(len+1), tag, get_stack_bottom()));
  drop(len);
  push(res);
}

void interpreter::eval_dup() {
  fill(2, pop());
}

void interpreter::eval_tag() {
  char *name = next_str();
  int32_t n  = next_int();
  int32_t t  = LtagHash(name);
  void *d    = reinterpret_cast<void*>(pop());
  push(Btag(d, t, box(n)));
}

void interpreter::eval_lstring() {
  push(reinterpret_cast<int32_t>(Lstring(reinterpret_cast<void*>(pop()))));
}

void interpreter::eval_lda(char l) {
  push(reinterpret_cast<int32_t>(get_by_location(l, next_int())));
}

void interpreter::eval_array() {
  int len = next_int();
  int32_t res = Barray_patt(reinterpret_cast<int32_t*>(pop()), box(len));
  push(res);
}

void interpreter::eval_fail() {
  int32_t a = next_int();
  int32_t b = next_int();
  failure("Explicitly failed with FAIL %d %d", a, b);
}

void interpreter::eval_closure() {
  int32_t shift = next_int();
  int32_t n_binded = next_int();
  int32_t binds[n_binded];
  for (int i = 0; i < n_binded; i++) {
    char l = next_char();
    int value = next_int();
    binds[i] = *get_by_location(l, value);
  }
  push(reinterpret_cast<int32_t>(Bclosure_my(box(n_binded), bf->code_ptr + shift, binds)));
}

void interpreter::eval_patt(char l) {
  int32_t* elem = reinterpret_cast<int32_t*>(pop());
  int32_t res;
  switch (l) {
    case 0:
      res = Bstring_patt(elem, reinterpret_cast<int32_t*>(pop()));
      break;
    case 1:
      res = Bstring_tag_patt(elem);
      break;
    case 2:
      res = Barray_tag_patt(elem);
      break;
    case 3:
      res = Bsexp_tag_patt(elem);
      break;
    case 4:
      res = Bboxed_patt(elem);
      break;
    case 5:
      res = Bunboxed_patt(elem);
      break;
    case 6:
      res = Bclosure_tag_patt(elem);
      break;
    default:
      failure("Unexpected PATT");
  }
  return push(res);
}

void interpreter::run() {
  FILE *f = stderr;

  do {
    char x = next_char(),
         h = (x & 0xF0) >> 4,
         l = x & 0x0F;

    auto fail = [h, l]() {
      failure ("ERROR: invalid opcode %d-%d\n", h, l);
    };
    
    switch (h) {
    case 15:
      return;
      
    case 0:
      eval_binop(l);
      break;
      
    case 1:
      switch (l) {
      case  0:
        eval_const();
        break;
        
      case  1:
        eval_string();
        break;
          
      case  2:
        eval_sexp();
        break;
        
      case  3:
        failure("STI instruction is deprecated");
        break;
        
      case  4:
        eval_sta();
        break;
        
      case  5:
        eval_jmp();
        break;
        
      case  6:
        eval_end();
        break;
        
      case  7:
        failure("behaviour of RET is undefined");
        break;
        
      case  8:
        eval_drop();
        break;
        
      case  9:
        eval_dup();
        break;
        
      case 10:
        failure("behaviour of SWAP is undefined");
        break;

      case 11:
        eval_elem();
        break;
        
      default:
        fail();
      }
      break;
      
    case 2:
    case 3:
    case 4:
      switch (h - 2) {
        case 0: eval_ld(l); break;
        case 1: eval_lda(l); break;
        case 2: eval_st(l); break;
      }
      break;
      
    case 5:
      switch (l) {
      case  0:
        eval_cjmp_z();
        break;
        
      case  1:
        eval_cjmp_nz();
        break;
        
      case  2:
        eval_begin();
        break;
        
      case  3:
        eval_cbegin();
        break;
        
      case  4:
        eval_closure();
        break;
          
      case  5:
        eval_callc();
        break;
        
      case  6:
        eval_call();
        break;
        
      case  7:
        eval_tag();
        break;
        
      case  8:
        eval_array();
        break;
        
      case  9:
        eval_fail();
        break;
        
      case 10:
        eval_line();
        break;

      default:
        fail();
      }
      break;
      
    case 6:
      eval_patt(l);
      break;

    case 7: {
      switch (l) {
      case 0:
        eval_read();
        break;
        
      case 1:
        eval_write();
        break;

      case 2:
        eval_length();
        break;

      case 3:
        eval_lstring();
        break;

      case 4:
        eval_barray();
        break;

      default:
        fail();
      }
    }
    break;
      
    default:
      fail();
    }
  }
  while (ip != nullptr);
}