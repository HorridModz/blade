# 
# @module clib
# 
# The `clib` module exposes Blade capabilites to interact with C 
# shared libraries. The workflow follows a simple approach.
# 
# - Load the library
# - Define the function schematics
# - Call the function.
# 
# That simple!
# 
# For example, the following code `dirname()` and `cos()` function from the 
# standard C library on a Unix machine (Linux, OSX, FreeBSD etc).
# 
# ```blade
# # Import clib
# import clib
# 
# # 1. Load 'libc' shared module available on Unix systems
# var lib = clib.load('libc')
# 
# # 2. Declare the functions
# var dirname = lib.define('dirname', clib.char_ptr, clib.char_ptr)
# var cos = lib.define('cos', clib.double, clib.double)     # this may not work on linux
# 
# # 3. Call the functions
# echo dirname('/path/to/my/file.ext')
# echo cos(23)
# 
# # Close the library (this is a good practice, but not required)
# lib.close()
# ```
# 
# The first argument to a definiton is the name of the function. 
# The second is its return type. If the function takes parameters, 
# the parameter types follow immediately. (See below for a list of the 
# available types.)
# 
# > **NOT YET SUPPORTED:**
# > - Variadic functions
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import .types { * }

import _clib
import os
import reflect

var _EXT = os.platform == 'windows' ? '.dll' : (
  os.platform == 'linux' ? '.so' : '.dylib'
)

/**
 * class CLib provides an interface for interacting with C shared modules.
 */
class Clib {
  var _ptr

  /**
   * CLib([name: string])
   * 
   * The name should follow the same practice outlined in `load()`.
   * @constructor
   */
  Clib(name) {
    if name != nil and !is_string(name)
      die Exception('string expected in argument 1 (name)')

    if name {
      self.load(name)
    }
  }

  _ensure_lib_loaded() {
    if !self._ptr
      die Exception('no library loaded')
  }

  /**
   * load(name: string)
   * 
   * Loads a new C shared library pointed to by name. Name must be a 
   * relative path, absolute path or the name of a system library. 
   * If the system shared library extension is omitted in the name, 
   * it will be automatically added except on Linux machines.
   * @return CLib
   */
  load(name) {
    if !is_string(name)
      die Exception('string expected in argument 1 (name)')
    if !name.ends_with(_EXT) and os.platform != 'linux'
      name += _EXT

    if self._ptr
      self.close()
    self._ptr = _clib.load(name)
  }

  /**
   * close()
   * 
   * Closes the handle to the shared library.
   */
  close() {
    self._ensure_lib_loaded()
    _clib.close(self._ptr)
  }

  /**
   * function(name: string)
   * 
   * Retrieves the handle to a specific function in the shared library.
   * @return ptr
   */
  function(name) {
    if !is_string(name)
      die Exception('string expected in argument 1 (name)')

    self._ensure_lib_loaded()
    return _clib.function(self._ptr, name)
  }

  /**
   * define(name: string, return_type: type, ...type)
   * 
   * Defines a new C function with the given name and return type.
   * -  When there are no more argument, it is declared that the function
   *    takes no argument.
   * -  `define()` expects a list of the argument/parameter types as expected
   *    by the function.
   * 
   * E.g.
   * 
   * ```blade
   * define('myfunc', int, int, ptr)
   * ```
   * 
   * Corresponds to the C declaration:
   * 
   * ```c
   * int myfunc(int a, void *b);
   * ```
   */
  define(name, return_type, ...) {
    if !is_string(name)
      die Exception('string expected in argument 1 (name)')

    # Ensure valid clib pointer.
    if !(reflect.is_ptr(return_type) and to_string(return_type).match('/clib/')) {
        die Exception('invalid return type')
    }

    var fn = self.function(name)
    var ffi_ptr = _clib.define(fn, name, return_type, __args__)

    def define(...) {
      return _clib.call(ffi_ptr, __args__)
    }

    return define
  }

  /**
   * get_pointer()
   * 
   * Returns a pointer to the underlying module
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }
}

/**
 * load(name: string)
 * 
 * Loads a new C shared library pointed to by name. Name must be a 
 * relative path, absolute path or the name of a system library. 
 * If the system shared library extension is omitted in the name, 
 * it will be automatically added.
 * @return CLib
 */
def load(name) {
  return Clib(name)
}

/**
 * new(type: type, ...any)
 * 
 * Creates a new C value for the specified clib type with the given values.
 * @returns bytes
 */
def new(type, ...) {
  if __args__.length() == 0
    die Exception('canot have an empty struct')

  # Ensure a valid and non void clib pointer.
  if !(reflect.is_ptr(type) and to_string(type).match('/clib/')) and type != void
    die Exception('invalid type for new')

  return _clib.new(type, __args__)
}

/**
 * get(type: type, data: string | bytes)
 * 
 * Returns the data contained in a C type _type_ encoded in the data.
 * The data should either be an output of `clib.new()` or a call to a 
 * function returning one of struct, union or array.
 * 
 * For structures created with `named_struct()`, a dictionary will 
 * automatically be returned with the values mapped to the names of the 
 * structure elements.
 * 
 * @returns list | dictionary
 */
def get(type, data) {
  # Ensure a valid and non void clib pointer.
  if !(reflect.is_ptr(type) and to_string(type).match('/clib/')) and type != void
    die Exception('invalid type for new')
  if is_string(data) data = data.to_bytes()

  return _clib.get(type, data)
}

/**
 * get_ptr_index(pointer: ptr, type: clib_type, index: number)
 * 
 * Get the value at the given index of a pointer based 
 * on the given CLib type.
 * 
 * @return any
 */
def get_ptr_index(pointer, type, index) {
  return _clib.get_ptr_index(pointer, type, index)
}

/**
 * get_ptr_index(pointer: ptr, type: clib_type, index: number, value: any)
 * 
 * Sets the value at the given index of a pointer based 
 * on the given CLib type to the given value.
 * 
 * @return any
 */
def set_ptr_index(pointer, type, index, value) {
  return _clib.set_ptr_index(pointer, type, index, value)
}
