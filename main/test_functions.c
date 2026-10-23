#include "test_functions.h"

void transmit_test_twai_frames(uint32_t no_of_frames, twai_node_handle_t twai_node_hdl) {

    for (uint32_t i=0; i < no_of_frames; i++) {
        size_t data_len = i % 9;

        uint8_t test_buf[8] = {0};

        for (uint8_t j=0; j < 8; j++) {
            test_buf[j] = i + j;
        }

        twai_frame_t test_twai_frame = {
            .header.id = i,
            .header.ide = false,
            .buffer = test_buf,
            .buffer_len = data_len,
            .header.dlc = data_len
        };

        ESP_ERROR_CHECK(twai_node_transmit(twai_node_hdl, &test_twai_frame, 0)); 
        ESP_ERROR_CHECK(twai_node_transmit_wait_all_done(twai_node_hdl, -1));
    }

}
