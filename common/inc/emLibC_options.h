/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EMLIBC_OPTIONS_H
#define __EMLIBC_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#define ATOMIC   0
#define REGUAL_STATELD   0 // Otherwise inverse flag to support 1 Active logic on pins
#define REDUCED_PAYLOAD  0 // Shrink state to 10 Bytes instead 40 Bytes

#ifdef __cplusplus
}
#endif

#endif /* __EMLIBC_OPTIONS_H */
