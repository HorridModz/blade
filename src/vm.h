#ifndef BLADE_VM_H
#define BLADE_VM_H

typedef struct s_compiler b_compiler;

#include "blob.h"
#include "config.h"
#include "object.h"
#include "table.h"
#include "value.h"

typedef enum {
  PTR_OK,
  PTR_COMPILE_ERR,
  PTR_RUNTIME_ERR,
} b_ptr_result;

typedef struct {
  uint16_t address;
  uint16_t finally_address;
  b_obj_class *klass;
} b_exception_frame;

typedef struct {
  b_obj_closure *closure;
  uint8_t *ip;
  b_value *slots;
  int handlers_count;
  b_exception_frame handlers[MAX_EXCEPTION_HANDLERS];
} b_call_frame;

struct s_vm_thread {
  pthread_mutex_t lock;
  int gc_protected;
  int frame_count;
  bool is_root;
  b_value stack[STACK_MAX];
  b_call_frame frames[FRAMES_MAX];
  b_value *stack_top;
  b_obj_up_value *open_up_values;
  b_obj_func *function;
  pthread_t *th;
  struct s_vm_thread *next;
};

struct s_vm {
  b_call_frame *current_frame;

  b_blob *blob;
  uint8_t *ip;

  b_obj *objects;
  b_compiler *compiler;
  b_obj_class *exception_class;
  char *root_file;

  // gc
  int gray_count;
  int gray_capacity;
  b_obj **gray_stack;
  size_t bytes_allocated;
  size_t next_gc;

  // objects tracker
  b_table modules;
  b_table strings;
  b_table globals;

  // object public methods
  b_table methods_string;
  b_table methods_list;
  b_table methods_dict;
  b_table methods_file;
  b_table methods_bytes;
  b_table methods_range;

  char **std_args;
  int std_args_count;

  // boolean flags
  bool is_repl;
  bool mark_value;
  // for switching through the command line args...
  bool show_warnings;
  bool should_debug_stack;
  bool should_print_bytecode;
  bool should_exit_after_bytecode;

  // miscellaneous
  b_vm_thread *thread;
};

void init_vm(b_vm *vm);

void free_vm(b_vm *vm);

b_ptr_result interpret_function(b_vm *vm, b_obj_func *function, b_vm_thread *thread);
b_ptr_result interpret(b_vm *vm, b_obj_module *module, const char *source);

void push(b_vm_thread *th, b_value value);
b_value pop(b_vm_thread *th);
b_value pop_n(b_vm_thread *th, int n);
b_value peek(b_vm_thread *th, int distance);

static inline void add_module(b_vm *vm, b_vm_thread *th, b_obj_module *module) {
  table_set(vm, &vm->modules, STRING_VAL(module->file), OBJ_VAL(module));
  if (th->frame_count == 0 || vm->current_frame == NULL) {
    table_set(vm, &vm->globals, STRING_VAL(module->name), OBJ_VAL(module));
  } else {
    table_set(vm,
              &vm->current_frame->closure->function->module->values,
              STRING_VAL(module->name), OBJ_VAL(module)
    );
  }
}

bool invoke_from_class(b_vm *vm, b_vm_thread *th, b_obj_class *klass, b_obj_string *name, int arg_count);

bool is_false(b_value value);

void dict_add_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);

bool dict_get_entry(b_obj_dict *dict, b_value key, b_value *value);

bool dict_set_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);

void define_native_method(b_vm *vm, b_vm_thread *th, b_table *table, const char *name, b_native_fn function);

bool is_instance_of(b_obj_class *klass1, char *klass2_name);

bool do_throw_exception(b_vm *vm, b_vm_thread *th, bool is_assert, const char *format, ...);

void do_runtime_error(b_vm *vm, b_vm_thread *th, const char *format, ...);

b_obj_instance *create_exception(b_vm *vm, b_vm_thread *th, b_obj_string *message);

#define EXIT_VM() return PTR_RUNTIME_ERR

#define runtime_error(...)                                                     \
  if(!throw_exception(__VA_ARGS__)){                                     \
    EXIT_VM(); \
  }

#define throw_exception(...) do_throw_exception(vm, th, false, ##__VA_ARGS__)

static inline b_obj *gc_protect(b_vm_thread *th, b_obj *object) {
  push(th, OBJ_VAL(object));
  th->gc_protected++;
  return object;
}

static inline void gc_clear_protection(b_vm_thread *th) {
  if (th->gc_protected > 0) {
    th->stack_top -= th->gc_protected;
  }
  th->gc_protected = 0;
}

// NOTE:
// any call to GC() within a function/block must be accompanied by
// at least one call to CLEAR_GC() before exiting the function/block
// otherwise, expected unexpected behavior
// NOTE as well that the call to CLEAR_GC() will be automatic for
// native functions.
// NOTE as well that METHOD_OBJECT must be retrieved before any call
// to GC() in a native function.
#define GC(o) gc_protect(th, (b_obj*)(o))
#define CLEAR_GC() gc_clear_protection(th)

b_vm_thread * new_vm_thread(b_vm *vm, bool is_root);
void free_vm_thread(b_vm_thread *thread);

#endif