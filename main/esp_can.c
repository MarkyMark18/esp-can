#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "test_functions.h"

#include "esp_log.h"
#include "esp_twai_onchip.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// FLAG FOR ENABLING BENCH TEST MODE
// WARNING: Do not uncomment if device is connected to car's CAN bus!
// #define BENCH_TEST_MODE

#ifdef BENCH_TEST_MODE
    #define TWAI_RX_PIN 47
    #define TWAI_TX_PIN 47
#else
    #define TWAI_RX_PIN 5
    #define TWAI_TX_PIN 6
#endif

#define CAN_OUTPUT_TASK_STACK_SIZE 4096 
#define CAN_OUTPUT_TASK_PRIORITY 1


#define TWAI_BITRATE 500000

typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
    bool is_rtr;
    bool is_extended_id;
} can_frame_t;

// Counter for tracking number of dropped frames (When queue is full)
static volatile uint32_t dropped_frame_ctr;

static QueueHandle_t can_frame_queue;

void can_output_task(void *pvParameters) {

    for(;;) {
        can_frame_t recd_frame;
        if (xQueueReceive(can_frame_queue, &recd_frame, (pdMS_TO_TICKS(1000)))) {
            char data_buf[28] = {0};
            size_t pos = 0;

            for (uint8_t i=0; i < recd_frame.dlc; i++) {
                int n = snprintf(data_buf + pos, sizeof(data_buf) - pos, "%02X ", recd_frame.data[i]);
                pos += n;
            }

            char output_buf[256] = {0};
            snprintf(
                output_buf, 
                sizeof(output_buf), 
                "ID: %" PRIX32 ", Data Len: %u, Data: %s\n",
                recd_frame.id,
                recd_frame.dlc,
                data_buf
            );

            printf("%s", output_buf);
        }

        else {
            printf("Current Dropped Frames: %" PRIu32 "\n", dropped_frame_ctr);
        }
    }
}

// TWAI receive callback
static bool twai_rx_cb(twai_node_handle_t node_hdl, const twai_rx_done_event_data_t *evt_data, void *user_ctx) {

    uint8_t recv_buff[8] = {0};
    twai_frame_t twai_rx_frame = {
        .buffer = recv_buff,
        .buffer_len = sizeof(recv_buff)
    };
    if (twai_node_receive_from_isr(node_hdl, (void *) &twai_rx_frame) == ESP_OK) {
        // Write details from received TWAI frame to CAN frame struct and add to queue
        can_frame_t can_rx_frame = {
            .id = twai_rx_frame.header.id,
            .dlc = twai_rx_frame.header.dlc,
            .is_rtr = twai_rx_frame.header.rtr,
            .is_extended_id = twai_rx_frame.header.ide
        };

        size_t data_len = twai_rx_frame.header.dlc < sizeof(can_rx_frame.data) ? twai_rx_frame.header.dlc : sizeof(can_rx_frame.data);

        memcpy(can_rx_frame.data, twai_rx_frame.buffer, data_len);

        if (!xQueueSendFromISR(can_frame_queue, &can_rx_frame, NULL)) {
            dropped_frame_ctr++;
        }
    }
    return false;
}


void app_main(void)
{   
    // CAN data frame queue creation
    can_frame_queue = xQueueCreate(
        100,
        sizeof(can_frame_t)
    );

    if (can_frame_queue == pdFAIL) {
        printf("Failed to create CAN frame queue!\n");
        return;
    }

    // CAN output task creation
    BaseType_t can_output_task_returned;

    can_output_task_returned = xTaskCreatePinnedToCore(
        can_output_task,
        "CAN Output Task",
        CAN_OUTPUT_TASK_STACK_SIZE,
        NULL,
        CAN_OUTPUT_TASK_PRIORITY,
        NULL,
        tskNO_AFFINITY
    );

    if (can_output_task_returned != pdPASS ) {
        printf("Failed to create CAN output task!!\n");
        return;
    }

    // Create a TWAI node for receiving CAN messages from the transceiver
    twai_node_handle_t twai_node_hdl = NULL;
    twai_onchip_node_config_t twai_node_cfg = {
        .io_cfg.rx = TWAI_RX_PIN,
        .bit_timing.bitrate = TWAI_BITRATE,
    };

    #ifdef BENCH_TEST_MODE
        printf("TEST MODE ENABLED! WARNING: DO NOT USE WHEN CONNECTED TO CAR\n");
        twai_node_cfg.io_cfg.tx = TWAI_TX_PIN,
        twai_node_cfg.flags.enable_self_test = true;
        twai_node_cfg.flags.enable_loopback = true;
        twai_node_cfg.tx_queue_depth = 5;
    #else
        printf("TEST MODE DISABLED! DEVICE WILL OPERATE IN LISTEN ONLY MODE\n");
        twai_node_cfg.flags.enable_listen_only = true;
    #endif

    ESP_ERROR_CHECK(twai_new_node_onchip(&twai_node_cfg, &twai_node_hdl));

    // Register the receive message callback

    twai_event_callbacks_t twai_cbs = {
        .on_rx_done = twai_rx_cb
    };

    ESP_ERROR_CHECK(twai_node_register_event_callbacks(twai_node_hdl, &twai_cbs, NULL));
    ESP_ERROR_CHECK(twai_node_enable(twai_node_hdl));

    #ifdef BENCH_TEST_MODE
        transmit_test_twai_frames(10000, twai_node_hdl);
    #endif

}
