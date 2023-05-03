#include "async.h"
#include <pthread.h>

void *blade_main_async_function(void *data) {
  b_obj_async *async = (b_obj_async *)data;
  async->running = true;
  b_ptr_result result = interpret_function(async->vm, async->function, true);
  async->running = false;
  async->completed = true;
  async->function->obj.stale = false;
  async->obj.stale = false;
  free_vm(async->vm);
  return NULL;
}

DECLARE_ASYNC_METHOD(start) {
  ENFORCE_ARG_COUNT(start, 0);
  b_obj_async *async = AS_ASYNC(METHOD_OBJECT);
  if(async->completed) {
    RETURN_ERROR("cannot start an exited async");
  } else if(async->running) {
    RETURN_ERROR("async already in running state");
  }

  async->obj.stale = true;
  async->function->obj.stale = true;
  int result = pthread_create(async->th, NULL, blade_main_async_function, (void *)async);
  if(result == 0) {
    RETURN_TRUE;
  }
  char * error = strerror(result);
  RETURN_STRING(error);
}

DECLARE_ASYNC_METHOD(join) {
  ENFORCE_ARG_COUNT(join, 0);
  b_obj_async *async = AS_ASYNC(METHOD_OBJECT);
  if(async->running) {
    int res = pthread_join(*async->th, NULL);
    if(res == 0) {
      RETURN_TRUE;
    }
    char * error = strerror(res);
    RETURN_STRING(error);
  }
  RETURN_TRUE;
}

DECLARE_ASYNC_METHOD(cancel) {
  ENFORCE_ARG_COUNT(cancel, 0);
  b_obj_async *async = AS_ASYNC(METHOD_OBJECT);
  if(async->running) {
    int res = pthread_cancel(*async->th);
    if(res == 0) {
      RETURN_TRUE;
    }
    char * error = strerror(res);
    RETURN_STRING(error);
  }
  RETURN_TRUE;
}

DECLARE_ASYNC_METHOD(state) {
  ENFORCE_ARG_COUNT(state, 0);
  b_obj_async *async = AS_ASYNC(METHOD_OBJECT);
  if(async->running) {
    RETURN_NUMBER(1);
  } else if(async->completed) {
    RETURN_NUMBER(2);
  }
  RETURN_NUMBER(0);
}

DECLARE_ASYNC_METHOD(copy) {
  ENFORCE_ARG_COUNT(copy, 0);
  b_obj_async *async = AS_ASYNC(METHOD_OBJECT);
  b_obj_async *n_async = (b_obj_async *)GC(new_async(vm, async->function));
  RETURN_OBJ(n_async);
}
