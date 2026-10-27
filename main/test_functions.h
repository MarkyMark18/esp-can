#ifndef TEST_FUNCTIONS_H
#define TEST_FUNCTIONS_H

#include "esp_twai.h"
#include "esp_err.h"

typedef enum {
    CAN_TEST_DROPPED_FRAMES,
    CAN_TEST_FRAME_DATA_BYTE_CHG
} can_test_mode_t;

void transmit_test_twai_frames(can_test_mode_t test_mode, twai_node_handle_t twai_node_hdl);

#endif