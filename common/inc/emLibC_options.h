/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EMLIBC_OPTIONS_H
#define __EMLIBC_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif
#include "options.h"
#define ATOMIC   0
#if OPTION_SHOW_TIMING == 0
#define REGUAL_STATELD  1  // Otherwise inverse flag to support 1 Active logic on pins
#else
#define REGUAL_STATELD  0 // Otherwise inverse flag to support 1 Active logic on pins
#endif
#define EMLIB_VERBOSE   0 // Otherwise inverse flag to support 1 Active logic on pins

#ifdef __cplusplus
}
#endif

#endif /* __EMLIBC_OPTIONS_H */
