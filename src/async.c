#include "async.h"
#include <pthread.h>

void *blade_main_async_function(void *data) {
  b_obj_async *async = (b_obj_async *)data;
//  async->running = true;
  interpret_function(async->vm, async->closure);
  async->running = false;
  async->completed = true;
//  async->closure->obj.stale = false;
//  async->obj.stale = false;
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

//  async->obj.stale = true;
//  async->closure->obj.stale = true;
  int result = pthread_create(async->th, NULL, blade_main_async_function, (void *)async);
  if(result == 0) {
    if(!async->completed) async->running = true;
    RETURN_TRUE;
  }
  RETURN_ERROR(strerror(result));
}

DECLARE_ASYNC_METHOD(join) {
  ENFORCE_ARG_COUNT(join, 0);
  b_obj_async *async = AS_ASYNC(METHOD_OBJECT);
  if(async->running) {
    int result = pthread_join(*async->th, NULL);
    if(result != 0) {
      RETURN_ERROR(strerror(result));
    }
  }
  RETURN_TRUE;
}

DECLARE_ASYNC_METHOD(cancel) {
  ENFORCE_ARG_COUNT(cancel, 0);
  b_obj_async *async = AS_ASYNC(METHOD_OBJECT);
  if(async->running) {
    int result = pthread_cancel(*async->th);
    if(result != 0) {
      RETURN_ERROR(strerror(result));
    }
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
  RETURN_OBJ(new_async(vm, async->closure));
}
