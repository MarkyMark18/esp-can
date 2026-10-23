idf_component_register(SRCS "esp_can.c" "test_functions.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_driver_twai)
