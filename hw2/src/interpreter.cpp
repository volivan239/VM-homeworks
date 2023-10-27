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

interpreter::interpreter(bytefile *bf, int32_t *&stack_top, int32_t *&stack_bottom):
  bf(bf), stack(callstack(stack_top, stack_bottom)), ip(bf->code_ptr) {}

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
    case 1: return stack.local(value);
    case 2: return stack.arg(value);
    case 3: return stack.closure_binded(value);
    default: inst_decode_failure();
  }
}

void interpreter::inst_decode_failure() {
  failure("ERROR: invalide opcode");
}

void interpreter::eval_binop(char l) {
  stack.binop(l);
}

void interpreter::eval_const() {
  stack.push(box(next_int()));
}

void interpreter::eval_end() {
  ip = stack.epilogue();
}

void interpreter::eval_drop() {
  stack.pop();
}

void interpreter::eval_st(char l) {
  int32_t ind = next_int();
  int32_t value = stack.pop();
  *get_by_location(l, ind) = value;
  stack.push(value);
}

void interpreter::eval_ld(char l) {
  int32_t ind = next_int();
  int32_t value = *get_by_location(l, ind);
  stack.push(value);
}

void interpreter::eval_begin() {
  int nargs = next_int();
  int nlocals = next_int();
  stack.prologue(nlocals, nargs);
}

void interpreter::eval_cbegin() {
  eval_begin();
}

void interpreter::eval_read() {
  stack.push(Lread());
}

void interpreter::eval_write() {
  stack.push(Lwrite(stack.pop()));
}

void interpreter::eval_line() {
  next_int();
}

void interpreter::eval_jmp() {
  ip = bf->code_ptr + next_int();
}

void interpreter::eval_cjmp_nz() {
  int32_t shift = next_int();
  if (unbox(stack.pop()) != 0) {
    ip = bf->code_ptr + shift;
  }
}

void interpreter::eval_cjmp_z() {
  int32_t shift = next_int();
  if (unbox(stack.pop()) == 0) {
    ip = bf->code_ptr + shift;
  }
}

void interpreter::eval_call() {
  int32_t shift = next_int();
  int32_t nargs = next_int();
  stack.reverse(nargs);
  stack.push(reinterpret_cast<int32_t>(ip));
  stack.push(nargs);
  ip = bf->code_ptr + shift;
}

void interpreter::eval_callc() {
  int32_t nargs = next_int();
  char* callee = reinterpret_cast<char*>(Belem(reinterpret_cast<int32_t*>(stack.nth(nargs)), box(0)));
  stack.reverse(nargs);
  stack.push(reinterpret_cast<int32_t>(ip));
  stack.push(nargs + 1);
  ip = callee;
}

void interpreter::eval_string() {
  stack.push(reinterpret_cast<int32_t>(Bstring(bf->string_ptr + next_int())));
}

void interpreter::eval_length() {
  stack.push(Llength(reinterpret_cast<void*>(stack.pop())));
}

void interpreter::eval_sta() {
  void *v = reinterpret_cast<void*>(stack.pop());
  int32_t i = stack.pop();
  if (is_boxed(i)) {   
    void *x = reinterpret_cast<void*>(stack.pop());
    stack.push(reinterpret_cast<int32_t>(Bsta(v, i, x))); 
  } else {
    stack.push(reinterpret_cast<int32_t>(Bsta(v, i, nullptr)));
  }
}

void interpreter::eval_elem() {
  int32_t v = stack.pop();
  void *p = reinterpret_cast<void*>(stack.pop());
  stack.push(reinterpret_cast<int32_t>(Belem(p, v)));
}

void interpreter::eval_barray() {
  int32_t len = next_int();
  stack.reverse(len);
  int32_t res = reinterpret_cast<int32_t>(Barray_my(box(len), stack.get_stack_bottom()));
  stack.drop(len);
  stack.push(res);
}

void interpreter::eval_sexp() {
  char *name = next_str();
  int32_t len = next_int();
  int32_t tag = LtagHash(name);
  stack.reverse(len);
  int32_t res = reinterpret_cast<int32_t>(Bsexp_my(box(len+1), tag, stack.get_stack_bottom()));
  stack.drop(len);
  stack.push(res);
}

void interpreter::eval_dup() {
  stack.fill(2, stack.pop());
}

void interpreter::eval_tag() {
  char *name = next_str();
  int32_t n  = next_int();
  int32_t t  = LtagHash(name);
  void *d    = reinterpret_cast<void*>(stack.pop());
  stack.push(Btag(d, t, box(n)));
}

void interpreter::eval_lstring() {
  stack.push(reinterpret_cast<int32_t>(Lstring(reinterpret_cast<void*>(stack.pop()))));
}

void interpreter::eval_lda(char l) {
  stack.push(reinterpret_cast<int32_t>(get_by_location(l, next_int())));
}

void interpreter::eval_array() {
  int len = next_int();
  int32_t res = Barray_patt(reinterpret_cast<int32_t*>(stack.pop()), box(len));
  stack.push(res);
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
  stack.push(reinterpret_cast<int32_t>(Bclosure_my(box(n_binded), bf->code_ptr + shift, binds)));
}

void interpreter::eval_patt(char l) {
  int32_t* elem = reinterpret_cast<int32_t*>(stack.pop());
  int32_t res;
  switch (l) {
    case 0:
      res = Bstring_patt(elem, reinterpret_cast<int32_t*>(stack.pop()));
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
  return stack.push(res);
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