#ifndef BLADE_SERIALIZE_H
#define BLADE_SERIALIZE_H

#include "compiler.h"

typedef enum {
  SERIALIZE_NIL,
  SERIALIZE_BOOL,
  SERIALIZE_NUMBER,
  SERIALIZE_STRING,
  SERIALIZE_FUNCTION,
  SERIALIZE_CLOSURE,
  SERIALIZE_SWITCH,
} b_serialize_type;

typedef struct {
  size_t count;
  size_t capacity;
  uint8_t *output;
} b_serial;

typedef struct {
  b_vm *vm;
  uint8_t *data;
  uint8_t *current;
  size_t count;
  b_blob blob;
  b_table modules;
} b_deserial;

void init_serial(b_serial *s);
void free_serial(b_vm *vm, b_serial *s);
b_serial *serialize(b_vm *vm, b_blob blob, char *filename);

b_deserial *new_deserial(b_vm *vm, uint8_t *data, size_t count);
void free_deserial(b_vm *vm, b_deserial *d);
b_blob deserialize(b_vm *vm, uint8_t *data, size_t length);

#endif //BLADE_SERIALIZE_H
