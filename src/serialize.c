#include <stdlib.h>
#include "serialize.h"
#include "memory.h"
#include "debug.h"
#include "module.h"

// forward declaration...
void serialize_blob(b_vm *vm, b_serial *s, b_blob blob);

void init_serial(b_serial *s){
  s->count = 0;
  s->capacity = 0;
  s->output = NULL;
}

void free_serial(b_vm *vm, b_serial *s) {
  if(s->capacity > 0 && s->output != NULL) {
    FREE_ARRAY(uint8_t, s->output, s->capacity);
  }
  init_serial(s);
}

void write_serial_byte(b_vm *vm, b_serial *s, uint8_t byte) {
  if (s->capacity < s->count + 1) {
    size_t old_capacity = s->capacity;
    s->capacity = GROW_CAPACITY(old_capacity);
    s->output = GROW_ARRAY(uint8_t, s->output, old_capacity, s->capacity);
  }

  s->output[s->count] = byte;
  s->count++;
}

void write_serial_bytes(b_vm *vm, b_serial *s, size_t size, uint8_t *bytes){
  if (s->capacity < s->count + size + 1) {
    size_t old_capacity = s->capacity;
    s->capacity = GROW_CAPACITY(old_capacity) + size;
    s->output = GROW_ARRAY(uint8_t, s->output, old_capacity, s->capacity);
  }

  memcpy(s->output + s->count, bytes, size);
  s->count += size;
}

void write_number_to_serial_bytes(b_vm *vm, b_serial *s, double number) {
  union {
    double d;
    uint8_t b[8];
  } tmp_union;
  tmp_union.d = number;
  write_serial_bytes(vm, s, 8, tmp_union.b);
}

void write_int_to_serial_byte(b_vm *vm, b_serial *s, int number) {
//#if IS_LITTLE_ENDIAN
//  write_serial_byte(vm, s, (uint8_t)number);
//  write_serial_byte(vm, s, (uint8_t)(number >> 8));
//  write_serial_byte(vm, s, (uint8_t)(number >> 16));
//  write_serial_byte(vm, s, (uint8_t)(number >> 24));
//#else
//  write_serial_byte(vm, s, (number >> 24) & 0xFF);
//  write_serial_byte(vm, s, (number >> 16) & 0xFF);
//  write_serial_byte(vm, s, (number >> 8) & 0xFF);
//  write_serial_byte(vm, s, number & 0xFF);
//#endif
  write_number_to_serial_bytes(vm, s, (double)number);
}

void write_string_to_serial_bytes(b_vm *vm, b_serial *s, b_obj_string *string) {
  write_int_to_serial_byte(vm, s, string->length);
  write_serial_bytes(vm, s, sizeof(char) * string->length, (uint8_t *)string->chars);
}

void write_function_to_serial_bytes(b_vm *vm, b_serial *s, b_obj_func *fn) {
  push(vm, OBJ_VAL(fn));
  write_int_to_serial_byte(vm, s, (int)fn->type);
  write_int_to_serial_byte(vm, s, fn->arity);
  write_int_to_serial_byte(vm, s, fn->up_value_count);
  write_serial_byte(vm, s, fn->is_variadic ? 1 : 0);
  if(fn->name != NULL) {
    write_string_to_serial_bytes(vm, s, fn->name);
  } else {
    write_string_to_serial_bytes(vm, s, copy_string(vm, "", 0));
  }
  write_string_to_serial_bytes(vm, s, copy_string(vm, fn->module->name, (int)strlen(fn->module->name)));
  write_string_to_serial_bytes(vm, s, copy_string(vm, fn->module->file, (int)strlen(fn->module->file)));

  b_serial n_serial;
  init_serial(&n_serial);
  serialize_blob(vm, &n_serial, fn->blob);
  write_number_to_serial_bytes(vm, s, (double)n_serial.count); // write the number of total bytes written.
  write_serial_bytes(vm, s, n_serial.count, n_serial.output);
  free_serial(vm, &n_serial);
  pop(vm);
}

void write_closure_to_serial_bytes(b_vm *vm, b_serial *s, b_obj_closure *closure) {
  push(vm, OBJ_VAL(closure));
  write_int_to_serial_byte(vm, s, closure->up_value_count);
  write_function_to_serial_bytes(vm, s, closure->function);
  pop(vm);
}

void write_switch_to_serial_bytes(b_vm *vm, b_serial *s, b_obj_switch *sw) {
}

void serialize_blob(b_vm *vm, b_serial *s, b_blob blob) {
  // blob count
  write_int_to_serial_byte(vm, s, blob.count);
  // blob lines
  for(int i = 0; i < blob.count; i++) {
    write_int_to_serial_byte(vm, s, blob.lines[i]);
  }
  // blob code
  write_serial_bytes(vm, s, sizeof(uint8_t) * blob.count, blob.code);

  // blob constants count
  write_int_to_serial_byte(vm, s, blob.constants.count);
  // blob constants
  for(int i = 0; i < blob.constants.count; i++) {
    b_value val = blob.constants.values[i];
    if(IS_NIL(val)) {
      write_int_to_serial_byte(vm, s, (int)SERIALIZE_NIL);
      write_serial_byte(vm, s, 0);
    } else if(IS_BOOL(val)) {
      write_int_to_serial_byte(vm, s, (int)SERIALIZE_BOOL);
      write_serial_byte(vm, s, AS_BOOL(val) ? 1 : 0);
    } else if(IS_NUMBER(val)) {
      write_int_to_serial_byte(vm, s, (int)SERIALIZE_NUMBER);
      write_number_to_serial_bytes(vm, s, AS_NUMBER(val));
    } else if(IS_STRING(val)) {
      write_int_to_serial_byte(vm, s, (int)SERIALIZE_STRING);
      write_string_to_serial_bytes(vm, s, AS_STRING(val));
    } else if(IS_FUNCTION(val)) {
      write_int_to_serial_byte(vm, s, (int)SERIALIZE_FUNCTION);
      write_function_to_serial_bytes(vm, s, AS_FUNCTION(val));
    } else if(IS_CLOSURE(val)) {
      write_int_to_serial_byte(vm, s, (int)SERIALIZE_CLOSURE);
      write_closure_to_serial_bytes(vm, s, AS_CLOSURE(val));
    } else if(IS_SWITCH(val)) {
      write_int_to_serial_byte(vm, s, (int)SERIALIZE_SWITCH);
      write_switch_to_serial_bytes(vm, s, AS_SWITCH(val));
    } else {
      // TODO: Return error that invalid constant cannot be serialized.
    }
  }
}

b_serial *serialize(b_vm *vm, b_blob blob, char *filename) {
  FILE *fp = fopen(filename, "w");
  if(!fp) {
    return NULL;
  }

  b_serial *serial = calloc(1, sizeof(b_serial));
  if(serial != NULL) {
    vm->is_serializing = true;

    init_serial(serial);
    serialize_blob(vm, serial, blob);

    bool successful_write = fwrite(serial->output, sizeof(uint8_t), serial->count, fp) > 0;
    fclose(fp);

    if(successful_write) {
//      TODO: remove testing...
      b_blob des_blob = deserialize(vm, serial->output, serial->count);
      b_obj_module *module = (b_obj_module *) GC(new_module(vm, strdup(""), strdup("<script>")));
      add_module(vm, module);
      CLEAR_GC();
      bind_native_modules(vm);
      interpret_blob(vm, module, des_blob);

      vm->is_serializing = false;
      return serial;
    }

    // we want to free the created serial before returning NULL.
    free_serial(vm, serial);
    // indicate we aren't serializing
    vm->is_serializing = false;
  }

  return NULL;
}

uint8_t read_byte(b_deserial *d) {
  return *d->current++;
}

uint8_t *read_bytes(b_deserial *d, size_t length) {
  uint8_t *res = calloc(length, sizeof(uint8_t));
  memcpy(res, d->current, length);
  d->current += length;
  return res;
}

double read_double(b_deserial *d) {
  union {
    double d;
    uint8_t b[8];
  } tmp_union;
  memcpy(tmp_union.b, d->current, 8);
  d->current += 8;
  return tmp_union.d;
}

int read_int(b_deserial *d) {
//  d->current += 4;
//  uint32_t p = d->current[-4];
//  uint32_t q = d->current[-3];
//  uint32_t r = d->current[-2];
//  uint32_t s = d->current[-1];
//#if IS_LITTLE_ENDIAN
//  return (int)(p | (q << 8) | (r << 16) | (s << 24));
//#else
//  return (int)((p << 24) | (q << 16) | (r << 8) | s);
//#endif
  return (int) read_double(d);
}

//int read_int(b_deserial *d) {
//#if IS_LITTLE_ENDIAN
//  return (int)(uint32_t)((uint32_t)d->current[*d->current++] |
//                         ((uint32_t)d->current[*d->current++] << 8) |
//                         ((uint32_t)d->current[*d->current++] << 16) |
//                         ((uint32_t)d->current[*d->current++] << 24));
//#else
//  return (int)(uint32_t)(((uint32_t)d->current[*d->current++] << 24) |
//      (d->current[*d->current++] << 16) |
//      ((uint32_t)d->current[*d->current++] << 8) |
//      (uint32_t)d->current[*d->current++]);
//#endif
//}

b_obj_string *read_string(b_deserial *d) {
  int length = read_int(d);
  char *string = calloc(length, sizeof(char));
  memcpy(string, d->current, length);
  d->current += length;
  return take_string(d->vm, string, length);
}

b_obj_func *read_function(b_deserial *d) {
  int type = read_int(d);
  int arity = read_int(d);
  int up_value_count = read_int(d);
  bool is_variadic = read_byte(d) == 1;
  b_obj_string *name = read_string(d);
  b_obj_string *module_name = read_string(d);
  b_obj_string *module_file = read_string(d);
  size_t blob_length = (size_t)read_double(d);

  uint8_t *tmp = calloc(blob_length, sizeof(uint8_t));
  memcpy(tmp, d->current, blob_length);
  d->current += blob_length;

  b_blob blob = deserialize(d->vm, tmp, blob_length);

  b_obj_module *module;
  b_value tmp_val;
  if(table_get(&d->vm->modules, OBJ_VAL(module_name), &tmp_val)) {
    module = AS_MODULE(tmp_val);
  } else {
    module = new_module(d->vm, module_name->chars, module_file->chars);
    add_module(d->vm, module);
  }

  b_obj_func *function = new_function(d->vm, module, type);
  function->arity = arity;
  function->up_value_count = up_value_count;
  function->is_variadic = is_variadic;
  function->module = module;
  if(name->length == 0) {
    function->name = NULL;
  } else {
    function->name = name;
  }
  function->blob = blob;
  return function;
}

b_obj_closure *read_closure(b_deserial *d) {
  int up_value_count = read_int(d);
  b_obj_func *fn = read_function(d);

  b_obj_closure *cls = new_closure(d->vm, fn);
  cls->up_value_count = up_value_count;
  return cls;
}

b_obj_switch *read_switch(b_deserial *d) {
  //  TODO: handle...
  return NULL;
}

b_deserial *new_deserial(b_vm *vm, uint8_t *data, size_t count) {
  b_deserial *d = calloc(1, sizeof(b_deserial));
  init_blob(&d->blob);
  d->vm = vm;
  d->count = count;
  d->data = data;
  d->current = data;
  init_table(&d->modules);
  return d;
}

void free_deserial(b_vm *vm, b_deserial *d) {
  if(d->data != NULL) {
    FREE_ARRAY(uint8_t, d->data, d->count);
  }
  // NOTE: we are not freeing the blob as it will be in use by the VM.
  free(d);
}

b_blob deserialize(b_vm *vm, uint8_t *data, size_t length) {
  bool was_serializing = vm->is_serializing;
  vm->is_serializing = true;
  b_deserial *des = new_deserial(vm, data, length);

  des->blob.count = read_int(des); // blob count
  des->blob.lines = calloc(des->blob.count, sizeof(int));
  for(int i = 0; i < des->blob.count; i++) {
    des->blob.lines[i] = read_int(des);
  }
  des->blob.code = read_bytes(des, des->blob.count);

  int constants_count = read_int(des);
  for(int i = 0; i < constants_count; i++) {
    b_serialize_type type = (b_serialize_type) read_int(des);
    switch (type) {
      case SERIALIZE_NIL: {
        add_constant(vm, &des->blob, NIL_VAL);
        break;
      }
      case SERIALIZE_BOOL: {
        add_constant(vm, &des->blob, BOOL_VAL(read_byte(des)));
        break;
      }
      case SERIALIZE_NUMBER: {
        add_constant(vm, &des->blob, NUMBER_VAL(read_double(des)));
        break;
      }
      case SERIALIZE_STRING: {
        add_constant(vm, &des->blob, OBJ_VAL(read_string(des)));
        break;
      }
      case SERIALIZE_FUNCTION: {
        add_constant(vm, &des->blob, OBJ_VAL(read_function(des)));
        break;
      }
      case SERIALIZE_CLOSURE: {
        add_constant(vm, &des->blob, OBJ_VAL(read_closure(des)));
        break;
      }
      case SERIALIZE_SWITCH: {
        add_constant(vm, &des->blob, OBJ_VAL(read_switch(des)));
        break;
      }
      default: {
        // TODO: return some out of the world error message
        break;
      }
    }
  }

//  free_deserial(vm, des);
  if(!was_serializing)
    vm->is_serializing = false;
  return des->blob;
}





















