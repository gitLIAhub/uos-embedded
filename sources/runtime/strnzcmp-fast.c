/*
FUNCTION
	<<strncmp>>---character string compare

INDEX
	strncmp

ANSI_SYNOPSIS
	#include <string.h>
	int strncmp(const char *<[a]>, const char * <[b]>, size_t <[length]>);

TRAD_SYNOPSIS
	#include <string.h>
	int strncmp(<[a]>, <[b]>, <[length]>)
	char *<[a]>;
	char *<[b]>;
	size_t <[length]>

DESCRIPTION
	<<strncmp>> compares up to <[length]> characters
	from the string at <[a]> to the string at <[b]>.

RETURNS
	If <<*<[a]>>> sorts lexicographically after <<*<[b]>>>,
	<<strncmp>> returns a number greater than zero.  If the two
	strings are equivalent, <<strncmp>> returns zero.  If <<*<[a]>>>
	sorts lexicographically before <<*<[b]>>>, <<strncmp>> returns a
	number less than zero.

PORTABILITY
<<strncmp>> is ANSI C.

<<strncmp>> requires no supporting OS subroutines.

QUICKREF
	strncmp ansi pure
*/
#include <runtime/lib.h>

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* DETECTNULL returns nonzero if (long)X contains a NULL byte. */
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)

small_int_t
strnzcmp(const unsigned char *s1, const unsigned char *s2, size_t limit)
{
  size_t n = limit;
  unsigned long *a1;
  unsigned long *a2;

  if (n == 0)
    return 0;

  /* If s1 or s2 are unaligned, then compare bytes. */
  if (!UNALIGNED (s1, s2))
    {
      /* If s1 and s2 are word-aligned, compare them a word at a time. */
      a1 = (unsigned long*)s1;
      a2 = (unsigned long*)s2;
      while (n >= sizeof (long) && *a1 == *a2)
        {
          /* If we've run out of bytes or hit a null, return zero
	     since we already know *a1 == *a2.  */
          if (n == 0 || DETECTNULL (*a1))
	    return 0;

          a1++;
          a2++;
          n -= sizeof (long);
        }

      /* A difference was detected in last few bytes of s1, so search bytewise */
      s1 = (unsigned char*)a1;
      s2 = (unsigned char*)a2;
    }

  while (n > 0)
    {
      if (__glibc_unlikely(*s1 != *s2)){
          if ((*s1 == '\0') || (*s2 == '\0'))
              return limit-n;
          return n - limit;
      }
      /* If we've run out of bytes or hit a null, return zero
	 since we already know *s1 == *s2.  */
      if (__glibc_unlikely(*s1 == '\0')) {
          return limit-n;
      }
      s1++;
      s2++;
      n--;
    }
  return limit;
}
