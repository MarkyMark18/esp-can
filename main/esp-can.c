#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#define CAN_OUTPUT_TASK_STACK_SIZE 4096 
#define CAN_OUTPUT_TASK_PRIORITY 1

#define TWAI_RX_PIN 17
#define TWAI_TX_PIN 17
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
            printf("Frame received with ID: %" PRIu32 "\n", recd_frame.id);
            printf("Frame data length: %u\n", recd_frame.dlc);
            printf("Frame data buffer contents: ");
            for (int i = 0; i < recd_frame.dlc; i++) {
                printf("%02X ", recd_frame.data[i]);
            }
            printf("\n");
        }
        else {
            printf("The queue is empty!\n");
        }
    }

}

// TWAI receive callback
static bool twai_rx_cb(twai_node_handle_t node_hdl, const twai_rx_done_event_data_t *evt_data, void *user_ctx) {

    uint8_t recv_buff[8];
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
        memcpy(can_rx_frame.data, twai_rx_frame.buffer, twai_rx_frame.header.dlc);

        if (!xQueueSendFromISR(can_frame_queue, &can_rx_frame, NULL)) {
            dropped_frame_ctr++;
            return false;
        }
    }
    return true;
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
    }

    // Create a TWAI node for receiving CAN messages from the transceiver
    // NOTE: In loopback mode currently for testing
    twai_node_handle_t twai_node_hdl = NULL;

    twai_onchip_node_config_t twai_node_cfg = {
        .io_cfg.tx = TWAI_TX_PIN,
        .io_cfg.rx = TWAI_RX_PIN,
        .bit_timing.bitrate = TWAI_BITRATE,
        .flags.enable_self_test = true,
        .flags.enable_loopback = true,
        .tx_queue_depth = 5
    };

    ESP_ERROR_CHECK(twai_new_node_onchip(&twai_node_cfg, &twai_node_hdl));

    // Register the receive message callback

    twai_event_callbacks_t twai_cbs = {
        .on_rx_done = twai_rx_cb
    };

    ESP_ERROR_CHECK(twai_node_register_event_callbacks(twai_node_hdl, &twai_cbs, NULL));

    ESP_ERROR_CHECK(twai_node_enable(twai_node_hdl));

    // Transmit a test TWAI frame
    uint8_t test_buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    twai_frame_t test_twai_frame = {
        .header.id = 0x1,
        .header.ide = false,
        .buffer = test_buf,
        .buffer_len = sizeof(test_buf),
        .header.dlc = sizeof(test_buf)
    };

    ESP_ERROR_CHECK(twai_node_transmit(twai_node_hdl, &test_twai_frame, 0));
    ESP_ERROR_CHECK(twai_node_transmit_wait_all_done(twai_node_hdl, -1));
}
