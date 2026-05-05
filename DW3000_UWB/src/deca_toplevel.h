#ifndef _DECA_TOPLEVEL_H_
#define _DECA_TOPLEVEL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DW3_DEFAULT_TIMEOUT 500

typedef enum
{
	DW3_OK = 0x00,
	DW3_TIMEDOUT = 0x01,
	DW3_ERROR = 0x02,
} DW3_StatusTypeDef;

void DW3_Init(void);
void DW3_Register(uint16_t tx_addr, uint16_t rx_addr);
DW3_StatusTypeDef DW3_StreamRX(double *pdistance, int16_t *pangle, uint32_t dw_timeout);
DW3_StatusTypeDef DW3_StreamTX(uint32_t dw_timeout);

#ifdef __cplusplus
}
#endif
#endif