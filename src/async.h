#ifndef BLADE_ASYNC_H
#define BLADE_ASYNC_H

#include "common.h"
#include "native.h"
#include "vm.h"

#define DECLARE_ASYNC_METHOD(name) DECLARE_METHOD(async##name)

/**
 * async.start()
 *
 * starts the background thread
 */
DECLARE_ASYNC_METHOD(start);

/**
 * async.join()
 *
 * joins the background thread and block until the thread finishes
 */
DECLARE_ASYNC_METHOD(join);

/**
 * async.cancel()
 *
 * cancels/exists the background thread
 */
DECLARE_ASYNC_METHOD(cancel);

/**
 * async.state()
 *
 * returns the state of the thread
 * 0 - Not started
 * 1 - Running
 * 2 - Completed
 */
DECLARE_ASYNC_METHOD(state);

/**
 * async.copy()
 *
 * returns a copy of this async that can run independent of it.
 */
DECLARE_ASYNC_METHOD(copy);

#endif // BLADE_ASYNC_H