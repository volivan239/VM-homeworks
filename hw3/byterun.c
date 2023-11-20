/* Lama SM Bytecode interpreter */

# include <string.h>
# include <errno.h>
# include <malloc.h>
# include <stdbool.h>
# include <stdlib.h>
# include <stdint.h>
# include <stdarg.h>
# include "byterun.h"

static void vfailure (char *s, va_list args) {
  fprintf(stderr, "*** FAILURE: ");
  vfprintf(stderr, s, args);   // vprintf (char *, va_list) <-> printf (char *, ...)
  exit(255);
}

void failure (char *s, ...) {
  va_list args;

  va_start(args, s);
  vfailure(s, args);
}

/* Gets a string from a string table by an index */
const char* get_string (bytefile *f, int pos) {
  return &f->string_ptr[pos];
}

/* Gets a name for a public symbol */
const char* get_public_name (bytefile *f, int i) {
  return get_string (f, f->public_ptr[i*2]);
}

/* Gets an offset for a publie symbol */
int get_public_offset (bytefile *f, int i) {
  return f->public_ptr[i*2+1];
}

/* Reads a binary bytecode file by name and unpacks it */
bytefile* read_file (char *fname) {
  FILE *f = fopen (fname, "rb");
  long size;
  bytefile *file;

  if (f == 0) {
    failure ("%s\n", strerror (errno));
  }
  
  if (fseek (f, 0, SEEK_END) == -1) {
    failure ("%s\n", strerror (errno));
  }

  file = (bytefile*) malloc (sizeof(int32_t)*5 + (size = ftell (f)));

  if (file == 0) {
    failure ("unable to allocate memory.\n");
  }
  
  rewind (f);

  if (size != fread (&file->stringtab_size, 1, size, f)) {
    failure ("%s\n", strerror (errno));
  }
  
  fclose (f);
  
  file->string_ptr    = &file->buffer [file->public_symbols_number * 2 * sizeof(int)];
  file->public_ptr    = (int32_t*) file->buffer;
  file->code_ptr      = &file->string_ptr [file->stringtab_size];
  file->global_ptr    = (int32_t*) malloc (file->global_area_size * sizeof (int));
  file->bytecode_size = (char*) &file->stringtab_size + size - file->code_ptr;

  return file;
}

void flog(FILE *f, const char *pat, ...) {
  if (f == NULL) {
    return;
  }

  va_list args;
  va_start(args, pat);
  vfprintf(f, pat, args);
}

const char* disassemble_one_instruction(FILE *f, bytefile *bf, const char *ip) {
# define INT    (ip += sizeof (int32_t), *(int32_t*)(ip - sizeof (int32_t)))
# define BYTE   *(ip++)
# define STRING get_string (bf, INT)
# define FAIL   failure ("ERROR: invalid opcode %d-%d\n", h, l)
  
  static const char * const ops [] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
  static const char * const pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
  static const char * const lds [] = {"LD", "LDA", "ST"};
  char x = BYTE,
        h = (x & 0xF0) >> 4,
        l = x & 0x0F;

  // fprintf (f, "0x%.8x:\t", (*ip)-bf->code_ptr-1);
  
  switch (h) {
  case 15:
    flog (f, "STOP");
    break;
    
  /* BINOP */
  case 0:
    flog (f, "BINOP\t%s", ops[l-1]);
    break;
    
  case 1:
    switch (l) {
    case  0:
      flog (f, "CONST\t%d", INT);
      break;
      
    case  1:
      flog (f, "STRING\t%s", STRING);
      break;
        
    case  2:
      flog (f, "SEXP\t%s ", STRING);
      flog (f, "%d", INT);
      break;
      
    case  3:
      flog (f, "STI");
      break;
      
    case  4:
      flog (f, "STA");
      break;
      
    case  5:
      flog (f, "JMP\t0x%.8x", INT);
      break;
      
    case  6:
      flog (f, "END");
      break;
      
    case  7:
      flog (f, "RET");
      break;
      
    case  8:
      flog (f, "DROP");
      break;
      
    case  9:
      flog (f, "DUP");
      break;
      
    case 10:
      flog (f, "SWAP");
      break;

    case 11:
      flog (f, "ELEM");
      break;
      
    default:
      FAIL;
    }
    break;
    
  case 2:
  case 3:
  case 4:
    flog (f, "%s\t", lds[h-2]);
    switch (l) {
    case 0: flog (f, "G(%d)", INT); break;
    case 1: flog (f, "L(%d)", INT); break;
    case 2: flog (f, "A(%d)", INT); break;
    case 3: flog (f, "C(%d)", INT); break;
    default: FAIL;
    }
    break;
    
  case 5:
    switch (l) {
    case  0:
      flog (f, "CJMPz\t0x%.8x", INT);
      break;
      
    case  1:
      flog (f, "CJMPnz\t0x%.8x", INT);
      break;
      
    case  2:
      flog (f, "BEGIN\t%d ", INT);
      flog (f, "%d", INT);
      break;
      
    case  3:
      flog (f, "CBEGIN\t%d ", INT);
      flog (f, "%d", INT);
      break;
      
    case  4:
      flog (f, "CLOSURE\t0x%.8x", INT);
      {int n = INT;
        for (int i = 0; i<n; i++) {
        switch (BYTE) {
          case 0: flog (f, "G(%d)", INT); break;
          case 1: flog (f, "L(%d)", INT); break;
          case 2: flog (f, "A(%d)", INT); break;
          case 3: flog (f, "C(%d)", INT); break;
          default: FAIL;
        }
        }
      };
      break;
        
    case  5:
      flog (f, "CALLC\t%d", INT);
      break;
      
    case  6:
      flog (f, "CALL\t0x%.8x ", INT);
      flog (f,  "%d", INT);
      break;
      
    case  7:
      flog (f, "TAG\t%s ", STRING);
      flog (f, "%d", INT);
      break;
      
    case  8:
      flog (f, "ARRAY\t%d", INT);
      break;
      
    case  9:
      flog (f, "FAIL\t%d", INT);
      flog (f, "%d", INT);
      break;
      
    case 10:
      flog (f, "LINE\t%d", INT);
      break;

    default:
      FAIL;
    }
    break;
    
  case 6:
    flog (f, "PATT\t%s", pats[l]);
    break;

  case 7: {
    switch (l) {
    case 0:
      flog (f, "CALL\tLread");
      break;
      
    case 1:
      flog (f, "CALL\tLwrite");
      break;

    case 2:
      flog (f, "CALL\tLlength");
      break;

    case 3:
      flog (f, "CALL\tLstring");
      break;

    case 4:
      flog (f, "CALL\tBarray\t%d", INT);
      break;

    default:
      FAIL;
    }
  }
  break;
    
  default:
    FAIL;
  }

  return ip;
}