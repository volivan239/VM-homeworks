# ifndef __INTERPRETER_STATE_H__
# define __INTERPRETER_STATE_H__
#include "bytefile.h"
#include "callstack.h"

class interpreter {
private:
  bytefile *bf;
  callstack stack;
  char *ip;

private:
  int32_t next_int();
  char next_char();
  char* next_str();

  int32_t* global(int32_t ind);
  int32_t* get_by_location(char l, int32_t value);

  void inst_decode_failure();

  void eval_binop(char l);
  void eval_const();
  void eval_end();
  void eval_drop();
  void eval_st(char l);
  void eval_ld(char l);
  void eval_begin();
  void eval_cbegin();
  void eval_read();
  void eval_write();
  void eval_line();
  void eval_jmp();
  void eval_cjmp_nz();
  void eval_cjmp_z();
  void eval_call();
  void eval_callc();
  void eval_string();
  void eval_length();
  void eval_sta();
  void eval_elem();
  void eval_barray();
  void eval_sexp();
  void eval_dup();
  void eval_tag();
  void eval_lstring();
  void eval_lda(char l);
  void eval_array();
  void eval_fail();
  void eval_closure();
  void eval_patt(char l);

  public:
  interpreter(bytefile *bf, int32_t *&stack_top, int32_t *&stack_bottom);

  void run();
};

# endif // __INTERPRETER_STATE_H__