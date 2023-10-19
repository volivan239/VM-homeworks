#include "interpreter.h"
#include <iostream>

extern "C" {
  extern void *alloc (size_t size);
  extern int Lread();
  extern int Lwrite(int);
  extern int Llength(void*);
  extern void* Bstring(void*);
  extern void* Belem (void *p, int i);
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

int32_t& interpreter::global(int32_t pos) {
  return bf->global_ptr[pos];
}

int32_t& interpreter::get_by_location(char l, int32_t value) {
  switch (l) {
    case 0: return global(value);
    case 1: return stack.local(value);
    case 2: return stack.arg(value);
    case 3: fprintf (stderr, "C(%d)", value); inst_decode_failure();
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
  get_by_location(l, ind) = value;
  stack.push(value);
}

void interpreter::eval_ld(char l) {
  int32_t ind = next_int();
  int32_t value = get_by_location(l, ind);
  stack.push(value);
}

void interpreter::eval_begin() {
  int nlocals = next_int();
  int nargs = next_int();
  stack.prologue(nlocals, nargs);
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
  int32_t _ = next_int();
  stack.push(reinterpret_cast<int32_t>(ip));
  ip = bf->code_ptr + shift;
}

void interpreter::eval_string() {
  stack.push(reinterpret_cast<int32_t>(Bstring(bf->string_ptr + next_int())));
}

void interpreter::eval_length() {
  stack.push(Llength(reinterpret_cast<void*>(stack.pop())));
}

void interpreter::eval_elem() {
  int32_t v = stack.pop();
  void *p = reinterpret_cast<void*>(stack.pop());
  stack.push(reinterpret_cast<int32_t>(Belem(p, v)));
}

void interpreter::run() {
  FILE *f = stderr;

  do {
    // std::cerr << stack.len() << std::endl;
    // fflush(f);
    char x = next_char(),
         h = (x & 0xF0) >> 4,
         l = x & 0x0F;

    auto fail = [h, l]() {
      failure ("ERROR: invalid opcode %d-%d\n", h, l);
    };

    // fprintf (f, "0x%.8x:\t", ip-bf->code_ptr-1);
    
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
        fprintf (f, "SEXP\t%s ", next_str());
        fprintf (f, "%d", next_int());
        break;
        
      case  3:
        fprintf (f, "STI");
        break;
        
      case  4:
        fprintf (f, "STA");
        break;
        
      case  5:
        eval_jmp();
        break;
        
      case  6:
        eval_end();
        break;
        
      case  7:
        fprintf (f, "RET");
        break;
        
      case  8:
        eval_drop();
        break;
        
      case  9:
        fprintf (f, "DUP");
        break;
        
      case 10:
        fprintf (f, "SWAP");
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
        case 1: fprintf(f, "LDA"); break;
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
        fprintf (f, "CBEGIN\t%d ", next_int());
        fprintf (f, "%d", next_int());
        break;
        
      case  4:
        fprintf (f, "CLOSURE\t0x%.8x", next_int());
        {int n = next_int();
         for (int i = 0; i<n; i++) {
         switch (next_char()) {
           case 0: fprintf (f, "G(%d)", next_int()); break;
           case 1: fprintf (f, "L(%d)", next_int()); break;
           case 2: fprintf (f, "A(%d)", next_int()); break;
           case 3: fprintf (f, "C(%d)", next_int()); break;
           default: fail();
         }
         }
        };
        break;
          
      case  5:
        fprintf (f, "CALLC\t%d", next_int());
        break;
        
      case  6:
        eval_call();
        break;
        
      case  7:
        fprintf (f, "TAG\t%s ", next_str());
        fprintf (f, "%d", next_int());
        break;
        
      case  8:
        fprintf (f, "ARRAY\t%d", next_int());
        break;
        
      case  9:
        fprintf (f, "FAIL\t%d", next_int());
        fprintf (f, "%d", next_int());
        break;
        
      case 10:
        eval_line();
        break;

      default:
        fail();
      }
      break;
      
    case 6:
      fprintf (f, "PATT\t%s", pats[l]);
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
        fprintf (f, "CALL\tLstring");
        break;

      case 4:
        fprintf (f, "CALL\tBarray\t%d", next_int());
        break;

      default:
        fail();
      }
    }
    break;
      
    default:
      fail();
    }

    // fprintf (f, "\n");
  }
  while (ip != nullptr);
  fprintf (f, "<end>\n");
}