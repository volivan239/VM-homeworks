# ifndef __CALLSTACK_H__
# define __CALLSTACK_H__
#include <inttypes.h>

class callstack {
private:
  int32_t *&stack_top;
  int32_t *&stack_bottom;
  int32_t *fp;

public:
  callstack(int32_t *&stack_top, int32_t *&stack_bottom);
  ~callstack();

  int32_t* get_stack_bottom();

  void push(int32_t value);
  int32_t pop_unchecked();
  int32_t pop();

  int32_t nth(int n);
  void drop(int n);
  void fill(int n, int32_t value);
  void reverse(int n);
  int32_t *get_current_closure();

  void prologue(int32_t nlocals, int32_t nargs);
  void cprologue(int32_t nlocals, int32_t nargs);
  char* epilogue();

  int32_t* local(int pos);
  int32_t* arg(int pos);
  int32_t* closure_binded(int pos);

  void binop(int opcode);
};

int32_t box(int32_t value);
int32_t unbox(int32_t value);
bool is_boxed(int32_t value);

# endif // __CALLSTACK_H__