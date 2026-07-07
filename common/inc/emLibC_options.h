/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EMLIBC_OPTIONS_H
#define __EMLIBC_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif
#include "options.h"
#define ATOMIC 0
#if OPTION_SHOW_TIMING == 1
#define REGUALR_STATELD  1 // Otherwise inverse flag to support 1 Active logic on pins
#else
#define REGUALR_STATELD  0 // Otherwise inverse flag to support 1 Active logic on pins
#endif

#define EMLIB_VERBOSE   0 // Otherwise inverse flag to support 1 Active logic on pins
#define REDUCED_PAYLOAD 0

#ifdef __cplusplus
}
#endif

#endif /* __EMLIBC_OPTIONS_H */
