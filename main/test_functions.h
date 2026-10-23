#ifndef TEST_FUNCTIONS_H
#define TEST_FUNCTIONS_H

#include "esp_twai.h"
#include "esp_err.h"

void transmit_test_twai_frames(uint32_t no_of_frames, twai_node_handle_t twai_node_hdl);

#endif