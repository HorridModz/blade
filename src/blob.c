#include "blob.h"
#include "memory.h"
#include "vm.h"

#include <stdlib.h>
#include <stdio.h>

void init_blob(b_blob *blob) {
  blob->count = 0;
  blob->capacity = 0;
  blob->code = NULL;
  blob->lines = NULL;
  init_value_arr(&blob->constants);
  pthread_mutex_init(&blob->lock, NULL);
}

void write_blob(b_vm *vm, b_blob *blob, uint8_t byte, int line) {
  pthread_mutex_lock(&blob->lock);

  if (blob->capacity < blob->count + 1) {
    int old_capacity = blob->capacity;
    blob->capacity = GROW_CAPACITY(old_capacity);
    blob->code = GROW_ARRAY(uint8_t, blob->code, old_capacity, blob->capacity);
    blob->lines = GROW_ARRAY(int, blob->lines, old_capacity, blob->capacity);
  }

  blob->code[blob->count] = byte;
  blob->lines[blob->count] = line;
  blob->count++;

  pthread_mutex_unlock(&blob->lock);
}

void free_blob(b_vm *vm, b_blob *blob) {
  SILENT(pthread_mutex_trylock(&blob->lock));

  if (blob->code != NULL) {
    FREE_ARRAY(uint8_t, blob->code, blob->capacity);
  }
  if (blob->lines != NULL) {
    FREE_ARRAY(int, blob->lines, blob->capacity);
  }
  free_value_arr(vm, &blob->constants);

  pthread_mutex_unlock(&blob->lock);
  pthread_mutex_destroy(&blob->lock);

  init_blob(blob);
}

int add_constant(b_vm *vm, b_blob *blob, b_value value) {
  push(vm->thread, value); // fixing gc corruption
  write_value_arr(vm, &blob->constants, value);
  pop(vm->thread); // fixing gc corruption
  return blob->constants.count - 1;
}
