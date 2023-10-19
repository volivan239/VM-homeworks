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

  void push(int32_t value);
  int32_t pop();
  void drop(int n);
  void reserve(int n);

  void prologue(int32_t nlocals, int32_t nargs);
  char* epilogue();

  int32_t& local(int pos);
  int32_t& arg(int pos);

  void binop(int opcode);

  int len() {
    return stack_top - stack_bottom;
  }
};

int32_t box(int32_t value);
int32_t unbox(int32_t value);

# endif // __CALLSTACK_H__