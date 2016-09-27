/* A very basic and small matrix algebra library atop libfixmath
 * fixed point numbers. Suitable for small matrices, usually less
 * than 10x10.
 *
 * This library does not do saturating arithmetic, but it does
 * feature an overflow flag for detecting erroneous results.
 *
 * Goals of the library are small footprint and fast execution.
 * Only very basic operations are supported.
 *
 * Matrices can have any size from 1x1 up to FIXMATRIX_MAX_SIZE
 * (configurable), also non-square, but memory is always allocated
 * for the maximum size.
 *
 * Error handling is done using flags in the matrix structure.
 * This makes it easy to detect if any errors occurred in any of
 * the computations, without checking a return status from each
 * function.
 *
 * The functions still perform the calculations even if the result
 * is known to be erroneous.
 */

#ifndef __fixmatrix_h_
#define __fixmatrix_h_

#include <stdint.h>
#include <stdbool.h>

#include "utility/math/uint32.h"
#include "utility/math/int64.h"
#include "utility/math/fract32.h"
#include "utility/math/fix16.h"
#include "utility/matrix/libmatrix.h"
#include "utility/filter/fixkalman.h"

#endif
