#include "interpreter.h"

extern "C" {
  extern void __init (void);
}

extern int32_t *__gc_stack_top, *__gc_stack_bottom;

void *__start_custom_data;
void *__stop_custom_data;

int main (int argc, char* argv[]) {
  __init();
  bytefile bf(argv[1]);
  interpreter interpreter_instance(&bf, __gc_stack_top, __gc_stack_bottom);
  interpreter_instance.run();
  return 0;
}