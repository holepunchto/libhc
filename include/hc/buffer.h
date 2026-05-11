#ifndef HC_BUFFER_H
#define HC_BUFFER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  size_t len;
  uint8_t *buffer;
} hc_buf_t;

#ifdef __cplusplus
}
#endif

#endif // HC_BUFFER_H
