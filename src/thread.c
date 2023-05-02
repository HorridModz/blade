#include "thread.h"
#include <pthread.h>

void *blade_main_thread_function(void *data) {
  b_obj_thread *thread = (b_obj_thread *)data;
  thread->running = true;
  interpret_function(thread->vm, thread->function, true);
  thread->running = false;
  thread->completed = true;
  thread->obj.stale = false;
  free_vm(thread->vm);
  return NULL;
}

DECLARE_NATIVE(thread) {
  ENFORCE_ARG_COUNT(thread, 1);
  ENFORCE_ARG_TYPE(thread, 0, IS_CLOSURE);
  RETURN_OBJ(new_thread(vm, AS_CLOSURE(args[0])->function));
}

DECLARE_THREAD_METHOD(start) {
  ENFORCE_ARG_COUNT(start, 0);
  b_obj_thread *thread = AS_THREAD(METHOD_OBJECT);
  if(thread->completed) {
    RETURN_ERROR("cannot start an exited thread");
  } else if(thread->running) {
    RETURN_ERROR("thread already in running state");
  }

  thread->obj.stale = true;
  int result = pthread_create(thread->th, NULL, blade_main_thread_function, (void *)thread);
  if(result == 0) {
    RETURN_TRUE;
  }
  char * error = strerror(result);
  RETURN_STRING(error);
}

DECLARE_THREAD_METHOD(join) {
  ENFORCE_ARG_COUNT(join, 0);
  b_obj_thread *thread = AS_THREAD(METHOD_OBJECT);
  if(thread->running) {
    int res = pthread_join(*thread->th, NULL);
    if(res == 0) {
      RETURN_TRUE;
    }
    char * error = strerror(res);
    RETURN_STRING(error);
  }
  RETURN_TRUE;
}

DECLARE_THREAD_METHOD(cancel) {
  ENFORCE_ARG_COUNT(cancel, 0);
  b_obj_thread *thread = AS_THREAD(METHOD_OBJECT);
  if(thread->running) {
    int res = pthread_cancel(*thread->th);
    if(res == 0) {
      RETURN_TRUE;
    }
    char * error = strerror(res);
    RETURN_STRING(error);
  }
  RETURN_TRUE;
}

DECLARE_THREAD_METHOD(state) {
  ENFORCE_ARG_COUNT(state, 0);
  b_obj_thread *thread = AS_THREAD(METHOD_OBJECT);
  if(thread->running) {
    RETURN_NUMBER(1);
  } else if(thread->completed) {
    RETURN_NUMBER(2);
  }
  RETURN_NUMBER(0);
}
