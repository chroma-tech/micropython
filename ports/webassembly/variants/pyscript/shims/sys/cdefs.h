#ifndef _SYS_CDEFS_H_
#define _SYS_CDEFS_H_

#include <stddef.h>
#include <stdint.h>

/* C++ externs */
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

/* Old-style prototype macro */
#ifndef __P
#define __P(protos) protos
#endif

/* No-op attrs */
#ifndef __dead
#define __dead
#endif
#ifndef __pure2
#define __pure2
#endif
#ifndef __unused
#define __unused
#endif
#ifndef __inline
#define __inline inline
#endif

/* ---- Minimal BSD sys/types shims used by BDB 1.xx ---- */
#ifndef _BSD_SYS_TYPES_SHIM_
#define _BSD_SYS_TYPES_SHIM_
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef unsigned short u_short;
typedef unsigned char u_char;
typedef char *caddr_t; /* for mmap-related members in btree.h */
#endif

#endif /* _SYS_CDEFS_H_ */
