#ifndef BLADE_THREAD_H
#define BLADE_THREAD_H

#include "common.h"
#include "native.h"
#include "vm.h"

#define DECLARE_THREAD_METHOD(name) DECLARE_METHOD(thread##name)


DECLARE_NATIVE(thread);

/**
 * thread.start()
 *
 * starts the background thread
 */
DECLARE_THREAD_METHOD(start);

/**
 * thread.join()
 *
 * joins the background thread and block until the thread finishes
 */
DECLARE_THREAD_METHOD(join);

/**
 * thread.cancel()
 *
 * cancels/exists the background thread
 */
DECLARE_THREAD_METHOD(cancel);

/**
 * thread.state()
 *
 * returns the state of the thread
 * 0 - Not started
 * 1 - Running
 * 2 - Completed
 */
DECLARE_THREAD_METHOD(state);

#endif // BLADE_THREAD_H