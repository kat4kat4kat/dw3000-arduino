// DW3 High-Level Wrapper
// Adapted from Qorvo's (formerly Decawave) DW3000 Series UWB Software Development Kit
// STM32 Platform Arduino Port

#ifdef __cplusplus
extern "C" {
#endif

#include <deca_toplevel.h>
#include <config_options.h>
#include <deca_probe_interface.h>
#include <deca_device_api.h>
#include <deca_spi.h>
#include <example_selection.h>
#include <port.h>
#include <shared_defines.h>
#include <shared_functions.h>
#include <Arduino.h>

#ifdef __cplusplus
}
#endif

#define RX_BUF_LEN 24

static void rx_ok_cb(const dwt_cb_data_t *cb_data);
static void rx_err_cb(const dwt_cb_data_t *cb_data);

extern dwt_config_t config_options;
extern dwt_txconfig_t txconfig_options;
extern dwt_txconfig_t txconfig_options_ch9;
static dwt_config_t config_ranging = {
    5,
    DWT_PLEN_64,
    DWT_PAC8,
    9,
    9,
    1,
    DWT_BR_6M8,
    DWT_PHRMODE_STD,
    DWT_PHRRATE_STD,
    129,
    DWT_STS_MODE_1 | DWT_STS_MODE_SDC,
    DWT_STS_LEN_64,
    DWT_PDOA_M3
};

GPIO_InitTypeDef   GPIO_InitStruct;
SPI_HandleTypeDef  hspi1;

SPI_HandleTypeDef   *hcurrent_active_spi = &hspi1;
uint16_t            pin_io_active_spi = DW_NSS_Pin;
GPIO_PinState       SPI_CS_state = GPIO_PIN_RESET;

uint16_t addr_dest = 0xABCD;
uint16_t addr_src = 0x1234;

void DW3_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = DW_IRQn_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(DW_IRQn_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DW_RESET_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(DW_RESET_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DW_IRQ2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(DW_IRQ2_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DW_NSS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(DW_NSS_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DW_NSS1_WAKEUP_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(DW_NSS1_WAKEUP_GPIO_Port, &GPIO_InitStruct);

	HAL_GPIO_WritePin(DW_RESET_GPIO_Port, DW_RESET_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(DW_NSS_GPIO_Port, DW_NSS_Pin, GPIO_PIN_SET);
    delay(2);
    HAL_GPIO_WritePin(DW_NSS_GPIO_Port, DW_NSS_Pin, GPIO_PIN_RESET);
    delay(2);
    HAL_GPIO_WritePin(DW_NSS_GPIO_Port, DW_NSS_Pin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(DW_NSS1_WAKEUP_GPIO_Port, DW_NSS1_WAKEUP_Pin, GPIO_PIN_RESET);
	
    __HAL_RCC_SPI1_CLK_ENABLE();
    GPIO_InitStruct.Pin = DW_SCK_Pin|DW_MISO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DW_MOSI_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(DW_MOSI_GPIO_Port, &GPIO_InitStruct);
    
	hspi1.Instance=SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&hspi1);
	
}

void DW3_Register(uint16_t tx_addr, uint16_t rx_addr)
{
    addr_dest = rx_addr;
    addr_src = tx_addr;
}

DW3_StatusTypeDef DW3_StreamRX(double *pdistance, int16_t *pangle, uint32_t dw_timeout)
{
    *pdistance = 0;
    *pangle = 0;

    #define TX_ANT_DLY 16385
    #define RX_ANT_DLY 16385

    uint8_t addr_dest_h = (uint8_t)(addr_dest >> 8U);
    uint8_t addr_dest_l = (uint8_t)(addr_dest & 0x00FF);
    uint8_t addr_src_h = (uint8_t)(addr_src >> 8U);
    uint8_t addr_src_l = (uint8_t)(addr_src & 0x00FF);
    uint8_t rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, addr_dest_l, addr_dest_h, addr_src_l, addr_src_h, 0x21};
    uint8_t tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, addr_src_l, addr_src_h, addr_dest_l, addr_dest_h, 0x10, 0x02, 0, 0};
    uint8_t rx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, addr_dest_l, addr_dest_h, addr_src_l, addr_src_h, 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    #define ALL_MSG_COMMON_LEN 10
    #define ALL_MSG_SN_IDX            2
    #define FINAL_MSG_POLL_TX_TS_IDX  10
    #define FINAL_MSG_RESP_RX_TS_IDX  14
    #define FINAL_MSG_FINAL_TX_TS_IDX 18
    static uint8_t frame_seq_nb = 0;
    #define RX_BUF_LEN 24
    static uint8_t rx_buffer[RX_BUF_LEN];
    static uint32_t status_reg = 0;
    #define POLL_RX_TO_RESP_TX_DLY_UUS 900
    #define RESP_TX_TO_FINAL_RX_DLY_UUS 670
    #define FINAL_RX_TIMEOUT_UUS 300
    #define PRE_TIMEOUT 0
    static uint64_t poll_rx_ts;
    static uint64_t resp_tx_ts;
    static uint64_t final_rx_ts;
    static double tof;
    static double distance;
    int range_ok = 0;
    uint32_t dw_tstart = 0;

    port_set_dw_ic_spi_fastrate();
    reset_DWIC();
    delay(2);
    dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf);

    dw_tstart = millis();
    while (!dwt_checkidlerc()) if (millis() - dw_tstart > dw_timeout) return DW3_TIMEDOUT;

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) return DW3_ERROR;
    if (dwt_configure(&config_ranging)) return DW3_ERROR;
    dwt_configuretxrf(&txconfig_options);
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    dw_tstart = millis();
    while (1)
    {
        if (millis() - dw_tstart > dw_timeout) return DW3_TIMEDOUT;
        
        dwt_setpreambledetecttimeout(0);
        dwt_setrxtimeout(0);
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        waitforsysstatus(&status_reg, NULL, (DWT_INT_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR), 0);

        if (status_reg & DWT_INT_RXFCG_BIT_MASK)
        {
            uint16_t frame_len;
            int goodSts = 0;
            int16_t stsQual;
            uint16_t stsStatus;

            dwt_writesysstatuslo(DWT_INT_RXFCG_BIT_MASK);

            if (((goodSts = dwt_readstsquality(&stsQual)) >= 0) && (dwt_readstsstatus(&stsStatus, 0) == DWT_SUCCESS))
            {
                frame_len = dwt_getframelength();
                if (frame_len <= RX_BUF_LEN)
                {
                    dwt_readrxdata(rx_buffer, frame_len, 0);
                }

                rx_buffer[ALL_MSG_SN_IDX] = 0;
                if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0)
                {
                    uint32_t resp_tx_time;
                    int ret;

                    poll_rx_ts = get_rx_timestamp_u64();

                    resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
                    dwt_setdelayedtrxtime(resp_tx_time);

                    dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
                    dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);

                    tx_resp_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
                    dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0);
                    dwt_writetxfctrl(sizeof(tx_resp_msg) + FCS_LEN, 0, 1);
                    dwt_setpreambledetecttimeout(PRE_TIMEOUT);
                    ret = dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);

                    if (ret == DWT_ERROR)
                    {
                        continue;
                    }

                    waitforsysstatus(&status_reg, NULL, (DWT_INT_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR), 0);

                    frame_seq_nb++;
                    if (status_reg & DWT_INT_RXFCG_BIT_MASK)
                    {
                        dwt_writesysstatuslo(DWT_INT_RXFCG_BIT_MASK | DWT_INT_TXFRS_BIT_MASK);

                        if (((goodSts = dwt_readstsquality(&stsQual)) >= 0) && (dwt_readstsstatus(&stsStatus, 0) == DWT_SUCCESS))
                        {
                            frame_len = dwt_getframelength();
                            if (frame_len <= RX_BUF_LEN)
                            {
                                dwt_readrxdata(rx_buffer, frame_len, 0);
                            }

                            rx_buffer[ALL_MSG_SN_IDX] = 0;
                            if (memcmp(rx_buffer, rx_final_msg, ALL_MSG_COMMON_LEN) == 0)
                            {
                                uint32_t poll_tx_ts, resp_rx_ts, final_tx_ts;
                                uint32_t poll_rx_ts_32, resp_tx_ts_32, final_rx_ts_32;
                                double Ra, Rb, Da, Db;
                                int64_t tof_dtu;
                                float angle_deg;

                                resp_tx_ts = get_tx_timestamp_u64();
                                final_rx_ts = get_rx_timestamp_u64();
                                angle_deg = ((float)dwt_readpdoa() / 2048.0F) * (180.0F / PI);

                                final_msg_get_ts(&rx_buffer[FINAL_MSG_POLL_TX_TS_IDX], &poll_tx_ts);
                                final_msg_get_ts(&rx_buffer[FINAL_MSG_RESP_RX_TS_IDX], &resp_rx_ts);
                                final_msg_get_ts(&rx_buffer[FINAL_MSG_FINAL_TX_TS_IDX], &final_tx_ts);

                                poll_rx_ts_32 = (uint32_t)poll_rx_ts;
                                resp_tx_ts_32 = (uint32_t)resp_tx_ts;
                                final_rx_ts_32 = (uint32_t)final_rx_ts;
                                Ra = (double)(resp_rx_ts - poll_tx_ts);
                                Rb = (double)(final_rx_ts_32 - resp_tx_ts_32);
                                Da = (double)(final_tx_ts - resp_rx_ts);
                                Db = (double)(resp_tx_ts_32 - poll_rx_ts_32);
                                tof_dtu = (int64_t)((Ra * Rb - Da * Db) / (Ra + Rb + Da + Db));

                                tof = tof_dtu * DWT_TIME_UNITS;
                                distance = tof * SPEED_OF_LIGHT;

                                *pdistance = abs(distance);
                                *pangle = angle_deg;

                                range_ok = 1;
                                break;
                            }
                        }
                    }
                    else
                    {
                        dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
                    }
                }
            }
        }
        else
        {
            dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        }

    }
	
	return DW3_OK;
	
}

DW3_StatusTypeDef DW3_StreamTX(uint32_t dw_timeout)
{
	#define TX_ANT_DLY 16385
    #define RX_ANT_DLY 16385

    uint8_t addr_dest_h = (uint8_t)(addr_dest >> 8U);
    uint8_t addr_dest_l = (uint8_t)(addr_dest & 0x00FF);
    uint8_t addr_src_h = (uint8_t)(addr_src >> 8U);
    uint8_t addr_src_l = (uint8_t)(addr_src & 0x00FF);
    uint8_t tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, addr_dest_l, addr_dest_h, addr_src_l, addr_src_h, 0x21};
    uint8_t rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, addr_src_l, addr_src_h, addr_dest_l, addr_dest_h, 0x10, 0x02, 0, 0};
    uint8_t tx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, addr_dest_l, addr_dest_h, addr_src_l, addr_src_h, 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    #define ALL_MSG_COMMON_LEN 10
    #define ALL_MSG_SN_IDX            2
    #define FINAL_MSG_POLL_TX_TS_IDX  10
    #define FINAL_MSG_RESP_RX_TS_IDX  14
    #define FINAL_MSG_FINAL_TX_TS_IDX 18
    static uint8_t frame_seq_nb = 0;
    #define RX_BUF_LEN 20
    static uint8_t rx_buffer[RX_BUF_LEN];
    static uint32_t status_reg = 0;
    #define POLL_TX_TO_RESP_RX_DLY_UUS 690
    #define RESP_RX_TO_FINAL_TX_DLY_UUS 880
    #define RESP_RX_TIMEOUT_UUS 300
    #define PRE_TIMEOUT 0
    static uint64_t poll_tx_ts;
    static uint64_t resp_rx_ts;
    static uint64_t final_tx_ts;
    uint32_t dw_tstart = 0;

    port_set_dw_ic_spi_fastrate();
    reset_DWIC();
    delay(2);
    dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf);

    dw_tstart = millis();
    while (!dwt_checkidlerc()) if (millis() - dw_tstart > dw_timeout) return DW3_TIMEDOUT;

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) return DW3_ERROR;
    if (dwt_configure(&config_ranging)) return DW3_ERROR;
    dwt_configuretxrf(&txconfig_options);
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT);
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);
    
    dw_tstart = millis();
    while (1)
    {
        if (millis() - dw_tstart > dw_timeout) return DW3_TIMEDOUT;

        tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
        dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
        dwt_writetxfctrl(sizeof(tx_poll_msg) + FCS_LEN, 0, 1);
        dwt_writesysstatuslo(0xFFFFFFFF);
        dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
        waitforsysstatus(&status_reg, NULL, (DWT_INT_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR), 0);

        frame_seq_nb++;
        if (status_reg & DWT_INT_RXFCG_BIT_MASK)
        {
            uint16_t frame_len;
            int goodSts = 0;
            int16_t stsQual;
            uint16_t stsStatus;

            dwt_writesysstatuslo(DWT_INT_RXFCG_BIT_MASK | DWT_INT_TXFRS_BIT_MASK);

            if (((goodSts = dwt_readstsquality(&stsQual)) >= 0) && (dwt_readstsstatus(&stsStatus, 0) == DWT_SUCCESS))
            {
                frame_len = dwt_getframelength();
                if (frame_len <= RX_BUF_LEN)
                {
                    dwt_readrxdata(rx_buffer, frame_len, 0);
                }

                rx_buffer[ALL_MSG_SN_IDX] = 0;
                if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
                {
                    uint32_t final_tx_time;
                    int ret;

                    poll_tx_ts = get_tx_timestamp_u64();
                    resp_rx_ts = get_rx_timestamp_u64();

                    final_tx_time = (resp_rx_ts + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
                    dwt_setdelayedtrxtime(final_tx_time);

                    final_tx_ts = (((uint64_t)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

                    final_msg_set_ts(&tx_final_msg[FINAL_MSG_POLL_TX_TS_IDX], poll_tx_ts);
                    final_msg_set_ts(&tx_final_msg[FINAL_MSG_RESP_RX_TS_IDX], resp_rx_ts);
                    final_msg_set_ts(&tx_final_msg[FINAL_MSG_FINAL_TX_TS_IDX], final_tx_ts);

                    tx_final_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
                    dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0);
                    dwt_writetxfctrl(sizeof(tx_final_msg) + FCS_LEN, 0, 1);
                    ret = dwt_starttx(DWT_START_TX_DELAYED);

                    if (ret == DWT_SUCCESS)
                    {
                        waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);
                        dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);
                        frame_seq_nb++;
                        break;
                    }
                }
            }
        }
        else
        {
            dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        }
    }

	return DW3_OK;

}

