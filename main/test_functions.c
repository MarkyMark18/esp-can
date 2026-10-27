#include "test_functions.h"

#define DROPPED_FRAME_TEST_FRAMES 10000
#define FRAME_DATA_BYTE_CHG_IDS 8 // Transmits 2 frames per ID

static void dropped_frame_test(twai_node_handle_t twai_node_hdl) {
    for (uint32_t i=0; i < DROPPED_FRAME_TEST_FRAMES; i++) {
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

static void frame_data_byte_chg_test(twai_node_handle_t twai_node_hdl) {
    size_t data_len = 8;
    uint8_t test_buf_original[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint8_t test_buf_changed[8] = {0x00, 0x01, 0x02, 0x03, 0x0F, 0x05, 0x06, 0x07};

    twai_frame_t test_frame_original = {
        .header.ide = false,
        .buffer = test_buf_original,
        .buffer_len = data_len,
        .header.dlc = data_len
    };

    twai_frame_t test_frame_changed = {
        .header.ide = false,
        .buffer = test_buf_changed,
        .buffer_len = data_len,
        .header.dlc = data_len
    };

    for (uint32_t i = 0; i < FRAME_DATA_BYTE_CHG_IDS; i++) {
        test_frame_original.header.id = i;

        ESP_ERROR_CHECK(twai_node_transmit(twai_node_hdl, &test_frame_original, 0));        
        ESP_ERROR_CHECK(twai_node_transmit_wait_all_done(twai_node_hdl, -1));
    }

    for (uint32_t i = 0; i < FRAME_DATA_BYTE_CHG_IDS; i++) {
        
        // Only change frames with even numbered IDs
        if (i % 2 == 0) {
            test_frame_changed.header.id = i;
            ESP_ERROR_CHECK(twai_node_transmit(twai_node_hdl, &test_frame_changed, 0));
        }
        else {
            test_frame_original.header.id = i;
            ESP_ERROR_CHECK(twai_node_transmit(twai_node_hdl, &test_frame_original, 0));
        }
        
        ESP_ERROR_CHECK(twai_node_transmit_wait_all_done(twai_node_hdl, -1));
    }
}

void transmit_test_twai_frames(can_test_mode_t test_mode, twai_node_handle_t twai_node_hdl) {

    switch (test_mode)
    {
    case CAN_TEST_DROPPED_FRAMES:
        dropped_frame_test(twai_node_hdl);
        break;

    case CAN_TEST_FRAME_DATA_BYTE_CHG:
        frame_data_byte_chg_test(twai_node_hdl);
        break;
    
    default:
        break;
    }

}
