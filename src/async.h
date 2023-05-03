#ifndef BLADE_ASYNC_H
#define BLADE_ASYNC_H

#include "common.h"
#include "native.h"
#include "vm.h"

#define DECLARE_THREAD_METHOD(name) DECLARE_METHOD(async##name)

DECLARE_NATIVE(thread);

/**
 * async.start()
 *
 * starts the background thread
 */
DECLARE_THREAD_METHOD(start);

/**
 * async.join()
 *
 * joins the background thread and block until the thread finishes
 */
DECLARE_THREAD_METHOD(join);

/**
 * async.cancel()
 *
 * cancels/exists the background thread
 */
DECLARE_THREAD_METHOD(cancel);

/**
 * async.state()
 *
 * returns the state of the thread
 * 0 - Not started
 * 1 - Running
 * 2 - Completed
 */
DECLARE_THREAD_METHOD(state);

#endif // BLADE_ASYNC_H