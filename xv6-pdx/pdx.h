/*
 * This file contains types and definitions for Portland State University.
 * The contents are intended to be visible in both user and kernel space.
 */

#ifndef PDX_INCLUDE
#define PDX_INCLUDE

#ifdef CS333_P2
#define ROOT_UID 0
#define ROOT_GID 0

#define UID_MIN 0
#define UID_MAX 32767

#define GID_MIN 0
#define GID_MAX 32767
#endif // CS333_P2

#define TRUE 1
#define FALSE 0
#define RETURN_SUCCESS 0
#define RETURN_FAILURE -1

#define NUL 0
#ifndef NULL
#define NULL NUL
#endif  // NULL

#define TPS 1000   // ticks-per-second
#define SCHED_INTERVAL (TPS/100)  // see trap.c

#define NPROC  64  // maximum number of processes -- normally in param.h

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#ifdef CS333_P3
// Anonymous nested function, ONLY works in GCC 7.4.0 or Greater
#define LAMBDA(c_) ({ c_ _;})
#endif // CS333_P3

#ifdef CS333_P4
#define MAXPRIO 6
#define DEFAULT_BUDGET 200
#define TICKS_TO_PROMOTE 15000
#endif //CS333_P4

#endif  // PDX_INCLUDE
