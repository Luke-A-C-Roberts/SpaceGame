#ifndef LDATA
#define LDATA

#include <stdint.h>
#include <stddef.h>

/*********DataTypes*********/
/*   Primative Datatypes   */
//-----------Ints----------//

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

//----------UInts----------//
 
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef size_t usize;
typedef ptrdiff_t isize;

//----------Float----------//

typedef long double f128;
typedef double f64;
typedef float f32;
// typedef _Complex long double cf128;
// typedef _Complex double cf64;
// typedef _Complex float cf32;

/*        Characters       */
//---------Strings---------//

typedef wchar_t wchar;
typedef char *str;
typedef wchar_t *wstr;
typedef char const*const strlit;
typedef wchar_t const*const wstrlit; 

/*          Other          */

typedef void* ptr;
 
/***************************/

#endif
