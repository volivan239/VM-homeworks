#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include "bytefile.h"

const char *ops [] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
const char *pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
const char *lds [] = {"LD", "LDA", "ST"};

char* bytefile::get_string( int pos) {
  return &string_ptr[pos];
}

char* bytefile::get_public_name(int i) {
  return get_string(public_ptr[i*2]);
}

int bytefile::get_public_offset(int i) {
  return public_ptr[i*2+1];
}

bytefile::bytefile(char *fname) {
  FILE *f = fopen (fname, "rb");

  if (f == 0) {
    failure ("%s\n", strerror (errno));
  }
  
  if (fseek (f, 0, SEEK_END) == -1) {
    failure ("%s\n", strerror (errno));
  }

  size_t size = ftell(f) - 3 * sizeof(int32_t);
  buffer = new char[size];

  if (buffer == 0) {
    failure ("*** FAILURE: unable to allocate memory.\n");
  }
  
  rewind (f);

  fread(&stringtab_size, sizeof(int), 1, f);
  fread(&global_area_size, sizeof(int), 1, f);
  fread(&public_symbols_number, sizeof(int), 1, f);
  if (size != fread (buffer, 1, size, f)) {
    failure ("%s\n", strerror (errno));
  }
  
  fclose (f);

  string_ptr = buffer + public_symbols_number * 2 * sizeof(int);
  public_ptr = reinterpret_cast<int*>(buffer);
  code_ptr   = string_ptr + stringtab_size;
  global_ptr = new int[global_area_size];
}

bytefile::~bytefile() {
  delete[] buffer;
  delete[] global_ptr;
}