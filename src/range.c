#include "range.h"

DECLARE_RANGE_METHOD(lower) {
  ENFORCE_ARG_COUNT(lower, 0);
  RETURN_NUMBER(AS_RANGE(METHOD_OBJECT)->lower);
}

DECLARE_RANGE_METHOD(upper) {
  ENFORCE_ARG_COUNT(upper, 0);
  RETURN_NUMBER(AS_RANGE(METHOD_OBJECT)->upper);
}

DECLARE_RANGE_METHOD(__iter__) {
  ENFORCE_ARG_COUNT(__iter__, 1);
  ENFORCE_ARG_TYPE(__iter__, 0, IS_NUMBER);

  b_obj_range *range = AS_RANGE(METHOD_OBJECT);

  int index = AS_NUMBER(args[0]);

  if (index >= 0 && index < range->range) {
    if(index == 0) RETURN_NUMBER(range->lower);
    RETURN_NUMBER(range->lower > range->upper ? --range->lower : ++range->lower);
  }

  RETURN_NIL;
}

DECLARE_RANGE_METHOD(__itern__) {
  ENFORCE_ARG_COUNT(__itern__, 1);
  b_obj_range *range = AS_RANGE(METHOD_OBJECT);

  if (IS_NIL(args[0])) {
    if (range->range == 0) {
      RETURN_NIL;
    }
    RETURN_NUMBER(0);
  }

  if (!IS_NUMBER(args[0])) {
    RETURN_ERROR("ranges are numerically indexed");
  }

  int index = (int)AS_NUMBER(args[0]) + 1;
  if (index < range->range) {
    RETURN_NUMBER(index);
  }

  RETURN_NIL;
}
