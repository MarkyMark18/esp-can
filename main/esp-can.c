#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#define CAN_OUTPUT_TASK_STACK_SIZE 4096 
#define CAN_OUTPUT_TASK_PRIORITY 1

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
        }
        else {
            printf("The queue is empty!\n");
        }
    }

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


    // TEST THAT QUEUE WORKS WITH A FAKE CAN FRAME
    can_frame_t test_frame = {
        .id = 999,
        .data = {0, 1, 2, 3, 4, 5, 6, 7},
        .dlc = 8,
        .is_rtr = false,
        .is_extended_id = false
    };

    xQueueSend(can_frame_queue, &test_frame, 0);
    
}
