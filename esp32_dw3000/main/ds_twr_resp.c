#include "dw3000_hw.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp_log.h>
#include "dw3000_hw.h"
#include "dw3000.h"
#include "dwhw.h"
#include "dwmac.h"
#include "dwphy.h"
#include "dwproto.h"
#include "ranging.h"
#include "dw3000_deca_regs.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "shared_functions.h"
#include "shared_defines.h"



/* Example application name */
#define APP_NAME "SIMPLE TX v1.0"

dwt_txconfig_t txconfig_options =
{
    0x34,           /* PG delay. */
    0xfdfdfdfd,      /* TX power. */
    0x0             /*PG count*/
};

static dwt_config_t config = {
    5,               /* 信道号. (Channel number.) */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* Tx前导码. (TX preamble code. Used in TX only.) */
    9,               /* Rx前导码. (RX preamble code. Used in RX only.) */
    1,               /* 帧分隔符模式. (0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type) */
    DWT_BR_6M8,      /* 数据速率. (Data rate.) */
    DWT_PHRMODE_STD, /* 物理层头模式. (PHY header mode.) */
    DWT_PHRRATE_STD, /* 物理层头速率. (PHY header rate.) */
    (129 + 8 - 8),   /* 帧分隔符超时. SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    DWT_STS_MODE_OFF,/* STS模式*/
    DWT_STS_LEN_64,  /* STS长度. (STS length, see allowed values in Enum dwt_sts_lengths_e) */
    DWT_PDOA_M0      /* PDOA mode off */
};

/* Example application name and version to print to the console. */
#define APP_NAME "DS TWR INIT v1.0"

/* If this define is enabled then the device will sleep->wakeup->restoreconfig*/
#define SLEEP_EN 0

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 1000

/* Default antenna delay values for 64 MHz PRF. See NOTE 1 below. */
#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8_t rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21, 0, 0};
static uint8_t tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0, 0,0,0,0, 0, 0};
static uint8_t rx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX            2
#define FINAL_MSG_POLL_TX_TS_IDX 10
#define FINAL_MSG_RESP_RX_TS_IDX 14
#define FINAL_MSG_FINAL_TX_TS_IDX 18
/* Frame sequence number, incremented after each transmission. */
static uint8_t frame_seq_nb = 0;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 24
static uint8_t rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32_t status_reg = 0;

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW IC's wait for response feature. */
#define POLL_RX_TO_RESP_TX_DLY_UUS 1500
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW IC's wait for response feature. */
#define RESP_TX_TO_FINAL_RX_DLY_UUS 500
/* Receive final timeout. See NOTE 5 below. */
#define FINAL_RX_TIMEOUT_UUS 220
/* Preamble timeout, in multiple of PAC size. See NOTE 7 below. */
#define PRE_TIMEOUT 5

/* Time-stamps of frames transmission/reception, expressed in device time units. */
static uint64_t poll_rx_ts;
static uint64_t resp_tx_ts;
static uint64_t final_rx_ts;

static double tof;
static double distance;



/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
 * temperature. These values can be calibrated prior to taking reference measurements. See NOTE 2 below. */

void ds_twr_responder()
{
     uint32_t dev_id;
    /* Configure SPI rate, DW3000 supports up to 38 MHz */

    dw3000_hw_init();
    dw3000_hw_reset();

    dw3000_spi_speed_fast();

    if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS) {
        ESP_LOGI("A", "DWT Probe Failed!");
        return ;
    }

    // 4. 初始化驱动 (这一步会分配内部内存，并安全地读取 Dev ID)
    if (dwt_initialise(DWT_LOADLDO) != DWT_SUCCESS) {
        ESP_LOGI("A", "DWT Initialise Failed!");
        return ;
    }

    dev_id = dwt_readdevid();
    if (dev_id != (uint32_t)DWT_DW3000_DEV_ID) {
        ESP_LOGI("A", "error dev_id");
        return ;
    }
    while (!dwt_checkidlerc()) {};


    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    if (dwt_configure(&config)) {
        ESP_LOGI("A", "DWT Configure Failed!");
        return ;
    }

    /* 配置DW3000发送频谱参数.*/
    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* 配置DW3000天线延迟参数.*/
    /* Apply default antenna delay value. See NOTE 1 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    uint32_t ranging_samples = 0;

    while (1) {
        /* 设置前导码超时. */
        dwt_setpreambledetecttimeout(0);
        /* 设置无接受超时时间. Clear reception timeout to start next ranging process. */
        dwt_setrxtimeout(0);

        /* 立即接受. Activate reception immediately. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll for reception of a frame or error/timeout. See NOTE 8 below. */
        waitforsysstatus(&status_reg, NULL, (DWT_INT_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR), 0);

        if (status_reg & DWT_INT_RXFCG_BIT_MASK) {
            uint16_t frame_len;

            /* Clear good RX frame event in the DW IC status register. */
            dwt_writesysstatuslo(DWT_INT_RXFCG_BIT_MASK);

            /* A frame has been received, read it into the local buffer. */
            frame_len = dwt_getframelength(0);
            if (frame_len <= RX_BUF_LEN)
            {
                dwt_readrxdata(rx_buffer, frame_len, 0);
                ESP_LOG_BUFFER_HEX("main", rx_buffer, frame_len - FCS_LEN);
            }

            /* Check that the frame is a poll sent by "DS TWR initiator" example.
             * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
            rx_buffer[ALL_MSG_SN_IDX] = 0;
            if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0)
            {
                ESP_LOGI("A", "111111111111111111");
                uint32_t resp_tx_time;
                int ret;

                /* Retrieve poll reception timestamp. */
                poll_rx_ts = get_rx_timestamp_u64();

                /* Set send time for response. See NOTE 9 below. */
                resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
                dwt_setdelayedtrxtime(resp_tx_time);

                /* Set expected delay and timeout for final message reception. See NOTE 4 and 5 below. */
                dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
                dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);
                /* Set preamble timeout for expected frames. See NOTE 6 below. */
                dwt_setpreambledetecttimeout(PRE_TIMEOUT);

                /* Write and send the response message. See NOTE 10 below.*/
                tx_resp_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
                dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
                dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1);          /* Zero offset in TX buffer, ranging. */
                ret = dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);

                /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 11 below. */
                if (ret == DWT_ERROR)
                {
                    ESP_LOGI("A", "DWT Start TX Failed!");
                    continue;
                }

                /* Poll for reception of expected "final" frame or error/timeout. See NOTE 8 below. */
                waitforsysstatus(&status_reg, NULL, (DWT_INT_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR), 0);

                /* Increment frame sequence number after transmission of the response message (modulo 256). */
                frame_seq_nb++;

                if (status_reg & DWT_INT_RXFCG_BIT_MASK)
                {
                    ESP_LOGI("A", "22222222222222222");
                    /* Clear good RX frame event and TX frame sent in the DW IC status register. */
                    dwt_writesysstatuslo(DWT_INT_RXFCG_BIT_MASK | DWT_INT_TXFRS_BIT_MASK);

                    /* A frame has been received, read it into the local buffer. */
                    frame_len = dwt_getframelength(0);
                    if (frame_len <= RX_BUF_LEN)
                    {
                        dwt_readrxdata(rx_buffer, frame_len, 0);
                    }

                    /* Check that the frame is a final message sent by "DS TWR initiator" example.
                     * As the sequence number field of the frame is not used in this example, it can be zeroed to ease the validation of the frame. */
                    rx_buffer[ALL_MSG_SN_IDX] = 0;
                    if (memcmp(rx_buffer, rx_final_msg, ALL_MSG_COMMON_LEN) == 0) {
                        ESP_LOGI("A", "333333333333333333");
                        uint32_t poll_tx_ts, resp_rx_ts, final_tx_ts;
                        uint32_t poll_rx_ts_32, resp_tx_ts_32, final_rx_ts_32;
                        double Ra, Rb, Da, Db;
                        int64_t tof_dtu;

                        /* Retrieve response transmission and final reception timestamps. */
                        resp_tx_ts = get_tx_timestamp_u64();
                        final_rx_ts = get_rx_timestamp_u64();

                        /* Get timestamps embedded in the final message. */
                        final_msg_get_ts(&rx_buffer[FINAL_MSG_POLL_TX_TS_IDX], &poll_tx_ts);
                        final_msg_get_ts(&rx_buffer[FINAL_MSG_RESP_RX_TS_IDX], &resp_rx_ts);
                        final_msg_get_ts(&rx_buffer[FINAL_MSG_FINAL_TX_TS_IDX], &final_tx_ts);

                        /* Compute time of flight. 32-bit subtractions give correct answers even if clock has wrapped. See NOTE 12 below. */
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
                        ranging_samples++;

                        /* Print computed distance on the console. */
                        char _dist_str[20];
                        sprintf(_dist_str, "%lu DIST: %3.2f m", ranging_samples, distance);
                        ESP_LOGI("ds_twr", "distant = %s", _dist_str);

                        /* as DS-TWR initiator is waiting for RNG_DELAY_MS before next poll transmission
                         * we can add a delay here before RX is re-enabled again
                         */
                        vTaskDelay(RNG_DELAY_MS - 10); // start couple of ms earlier
                    }
                } else {
                    /* Clear RX error/timeout events in the DW IC status register. */
                    dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
                }
            }
        } else {
            ESP_LOGI("ds_twr", "RX error/timeout");
            /* Clear RX error/timeout events in the DW IC status register. */
            dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        }
    }
}



