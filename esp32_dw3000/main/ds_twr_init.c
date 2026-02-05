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
static uint8_t tx_poll_msg[] = { 0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21 };
static uint8_t rx_resp_msg[] = { 0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0 };
static uint8_t tx_final_msg[] = { 0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX            2
#define FINAL_MSG_POLL_TX_TS_IDX  10
#define FINAL_MSG_RESP_RX_TS_IDX  14
#define FINAL_MSG_FINAL_TX_TS_IDX 18
/* Frame sequence number, incremented after each transmission. */
static uint8_t frame_seq_nb = 0;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 20
static uint8_t rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32_t status_reg = 0;

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW IC's wait for response feature. */
#define POLL_TX_TO_RESP_RX_DLY_UUS (700)
/* This is the delay from Frame RX timestamp to TX reply timestamp used for calculating/setting the DW IC's delayed TX function.
 * This value is required to be larger than POLL_TX_TO_RESP_RX_DLY_UUS. Please see NOTE 4 for more details. */
#define RESP_RX_TO_FINAL_TX_DLY_UUS (700)
/* Receive response timeout. See NOTE 5 below. */
#define RESP_RX_TIMEOUT_UUS 300
/* Preamble timeout, in multiple of PAC size. See NOTE 7 below. */
#define PRE_TIMEOUT 5

/* Time-stamps of frames transmission/reception, expressed in device time units. */
static uint64_t poll_tx_ts;
static uint64_t resp_rx_ts;
static uint64_t final_tx_ts;


/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
 * temperature. These values can be calibrated prior to taking reference measurements. See NOTE 2 below. */

void ds_twr_initiator()
{
     uint32_t dev_id;
    /* Configure SPI rate, DW3000 supports up to 38 MHz */

    dw3000_hw_init();
    dw3000_hw_reset();

    dw3000_spi_speed_fast();

    if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS) {
        ESP_LOGI("TAG", "DWT Probe Failed!");
        return ;
    }

    // 4. 初始化驱动 (这一步会分配内部内存，并安全地读取 Dev ID)
    if (dwt_initialise(DWT_LOADLDO) != DWT_SUCCESS) {
        ESP_LOGI("TAG", "DWT Initialise Failed!");
        return ;
    }

    dev_id = dwt_readdevid();
    if (dev_id != (uint32_t)DWT_DW3000_DEV_ID) {
        ESP_LOGI("TAG", "error dev_id");
        return ;
    }
    while (!dwt_checkidlerc()) {};


    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    if (dwt_configure(&config)) {
        ESP_LOGI("TAG", "DWT Configure Failed!");
        return ;
    }

    /* 配置DW3000发送频谱参数.*/
    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* 配置DW3000天线延迟参数.*/
    /* Apply default antenna delay value. See NOTE 1 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    /* 配置DW3000发送完成后POLL_TX_TO_RESP_RX_DLY_UUS开启接受. Set expected response's delay and timeout. See NOTE 4, 5 and 7 below.
     * As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    /* 配置DW3000接受超时时间RESP_RX_TIMEOUT_UUS. */
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    /* 配置前导码超时时间PRE_TIMEOUT*/
    dwt_setpreambledetecttimeout(PRE_TIMEOUT);

    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    while (1) {
        /* 写入待发送数据到DW3000准备发送,并设置发送长度. Write frame data to DW IC and prepare transmission. See NOTE 9 below. */
        tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
        dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_poll_msg)+FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging. */
    
        /* 立即发送发送，并有相应. Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay
         * set by dwt_setrxaftertxdelay() has elapsed. */
        if (dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
            ESP_LOGI("TAG", "DWT Start TX Failed!");
            continue;
        }

        /* 查询DW3000是否接受成功|接受超时|接受错误. We assume that the transmission is achieved correctly, poll for reception of a frame or error/timeout. See NOTE 10 below. */
        /* We assume that the transmission is achieved correctly, poll for reception of a frame or error/timeout. See NOTE 10 below. */
        waitforsysstatus(&status_reg, NULL, (DWT_INT_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR), 0);

        frame_seq_nb++;

        if (status_reg & DWT_INT_RXFCG_BIT_MASK) { 
            ESP_LOGI("TAG", "111111111111");
            uint16_t frame_len;

            /* Clear good RX frame event and TX frame sent in the DW IC status register. */
            dwt_writesysstatuslo(DWT_INT_RXFCG_BIT_MASK | DWT_INT_TXFRS_BIT_MASK);

            /* A frame has been received, read it into the local buffer. */
            frame_len = dwt_getframelength(0);

            if (frame_len <= RX_BUF_LEN)
                dwt_readrxdata(rx_buffer, frame_len, 0);

            rx_buffer[ALL_MSG_SN_IDX] = 0;
            if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0) {
                ESP_LOGI("TAG", "222222222222");
                uint32_t final_tx_time;
                int ret;

                /* 获取Poll发送时间戳+Resp接受时间戳. Retrieve poll transmission and response reception timestamp. */
                poll_tx_ts = get_tx_timestamp_u64();
                resp_rx_ts = get_rx_timestamp_u64();

                /* 计算Final发送时间戳. Compute final message transmission time. See NOTE 11 below. */
                final_tx_time = (resp_rx_ts + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
                dwt_setdelayedtrxtime(final_tx_time);

                /* Final TX timestamp is the transmission time we programmed plus the TX antenna delay. */
                final_tx_ts = (((uint64_t)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

                /* 写入到数据帧中,打包发送. Write all timestamps in the final message. See NOTE 12 below. */
                final_msg_set_ts(&tx_final_msg[FINAL_MSG_POLL_TX_TS_IDX], poll_tx_ts);
                final_msg_set_ts(&tx_final_msg[FINAL_MSG_RESP_RX_TS_IDX], resp_rx_ts);
                final_msg_set_ts(&tx_final_msg[FINAL_MSG_FINAL_TX_TS_IDX], final_tx_ts);

                /* 写入待发送数据到DW3000准备发送,并设置发送长度. Write and send final message. See NOTE 9 below. */
                tx_final_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
                dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0); /* Zero offset in TX buffer. */
                dwt_writetxfctrl(sizeof(tx_final_msg)+FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging bit set. */
                /* 延迟发送 */
                ret = dwt_starttx(DWT_START_TX_DELAYED);

                if (ret == DWT_SUCCESS) {
                    ESP_LOGI("TAG", "333333333333");
                    waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);

                    /* Clear TXFRS event. */
                    dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);

                    /* Increment frame sequence number after transmission of the final message (modulo 256). */
                    frame_seq_nb++;
                } else {
                    ESP_LOGI("TAG", "Erroe333333333333");

                }
            } else {
                ESP_LOGI("TAG", "Erroe222222222222");
            }
        } else {
            ESP_LOGI("TAG", "Erroe11111111111");
            /* Clear RX error/timeout events in the DW IC status register. */
            dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR | DWT_INT_TXFRS_BIT_MASK);
        }
        vTaskDelay(RNG_DELAY_MS);
    }

}



