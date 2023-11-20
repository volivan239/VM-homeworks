# include <stdio.h>

/* The unpacked representation of bytecode file */
typedef struct {
  const char     * string_ptr;              /* A pointer to the beginning of the string table */
  const int32_t  * public_ptr;              /* A pointer to the beginning of publics table    */
  const char     * code_ptr;                /* A pointer to the bytecode itself               */
  const int32_t  * global_ptr;              /* A pointer to the global area                   */
  int   bytecode_size;
  int   stringtab_size;          /* The size (in bytes) of the string table        */
  int   global_area_size;        /* The size (in words) of global area             */
  int   public_symbols_number;   /* The number of public symbols                   */
  char  buffer[0];               
} bytefile;

bytefile* read_file (char *fname);
const char* disassemble_one_instruction(FILE *f, bytefile *bf, const char *ip);