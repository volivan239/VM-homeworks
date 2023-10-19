# ifndef __BYTECODE_LOADER_H__
# define __BYTECODE_LOADER_H__

extern "C" {
  #include "runtime.h"
}

extern const char *ops [];
extern const char *pats[];
extern const char *lds [];

/* The unpacked representation of bytecode file */
class bytefile {
private:
  int   stringtab_size;          /* The size (in bytes) of the string table        */
  int   global_area_size;        /* The size (in words) of global area             */
  int   public_symbols_number;   /* The number of public symbols                   */
  char  *buffer;

public:
  char *string_ptr;              /* A pointer to the beginning of the string table */
  int  *public_ptr;              /* A pointer to the beginning of publics table    */
  char *code_ptr;                /* A pointer to the bytecode itself               */
  int  *global_ptr;              /* A pointer to the global area                   */

  bytefile(char *fname);
  ~bytefile();

  char* get_string (int pos);
  char* get_public_name (int i);
  int get_public_offset (int i);

};
# endif // __BYTECODE_LOADER_H__