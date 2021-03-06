#ifndef __POSIX_PORT__
#define __POSIX_PORT__

#ifdef __cplusplus
extern "C" {
#endif



//#define POSIX_memory	uos_memory

#define UOS_USLEEP_STYLE_DELAY          0
#define UOS_USLEEP_STYLE_ETIEMER_SLEEP  1
#define UOS_USLEEP_STYLE    UOS_USLEEP_STYLE_DELAY

/*
 *#define __UOS_STDIO_IS_NULL    0
 *#define __UOS_STDIO_IS_STREAM  1
 */
//#define __UOS_STDIO__ __UOS_STDIO_IS_???

//#define POSIX_timer uos_timer

//INLINE unixtime32_t time(unixtime32_t* t) __THROW ???
//#define UOS_HAVE_UNIXTIME

//#define TASK_PRIORITY_MAX 100
//#define TASK_PRIORITY_MIN 0

//#define _SC_PAGE_SIZE

//* choose wich new will used:
    //from stdlibc++
#define UOS_POSIX_NEW_LIBC      0
    //from local uos ustdlib
#define UOS_POSIX_NEW_UOS       1
#define UOS_POSIX_NEW           UOS_POSIX_NEW_UOS

//* reimplements NewLib stdlibc stdin/out/err streams for routines build against newlib
//* it froces allocate default reent structure
#define UOS_POSIX_NEWLIB_IO     0

#ifdef __cplusplus
}
#endif

#endif //__POSIX_PORT__

