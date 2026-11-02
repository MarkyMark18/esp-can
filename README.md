# esp-can

A simple CAN bus frame sniffing interface, based on the ESP32-S3 and written in C using ESP-IDF and FreeRTOS.

Initial goals are to sniff CAN frames from my car's (BMW E87) PT-CAN, to learn more about CAN and with a view to use this information in future projects.

## Status

- Currently tested in self-test loopback mode with 10000 test frames
- Hasn't been wired into a car yet
- **IMPORTANT: Test frame setup currently transmits large numbers of frames over RX pin. Do not use on a live car.**

## What It Does & How It Works

- Receives frames from the TWAI controller and unpacks them into can_frame_t structs, then adds them to a FreeRTOS queue to minimise work done inside ISR
  - Frames are queued by value to avoid dangling pointers
  - Queue size is fixed to 100 items and number of frames dropped when queue is full is logged by dropped_frame_ctr
- A FreeRTOS task then retrieves frames from the queue and (See Example Output below for formatting):
  - Prints the individual frame to the monitor (If PRINT_INDV_FRAMES is defined)
  - Prints the CAN frame table to the monitor every 5 seconds
- Test function transmit_test_twai_frames() has 2 modes:
  - CAN_TEST_DROPPED_FRAMES - Stress testing using the number of frames defined in DROPPED_FRAME_TEST_FRAMES
  - CAN_TEST_FRAME_DATA_BYTE_CHG - Transmits 2 frames per ID to test storing of previous data. Number of IDs transmitted defined in FRAME_DATA_BYTE_CHG_IDS
- Console output is set to USB Serial/JTAG to avoid frames dropping due to slow UART writes
- Compile time switch added to allow test bench and live car (listen only) modes.

## Hardware

I am using an Arduino Nano ESP32, with GPIO pin 47 used for both TWAI TX and RX in test bench mode, to enable loopback. Pins 5 (RX) and 6 (TX) have been assigned for use when connecting to the car as a precaution against accidentally transmitting CAN frames to car's PT-CAN.

## Building & Flashing

- Build using ESP-IDF v6.1 (set-target esp32s3, build, flash monitor)

## Configuration

- Use the sdkconfig that is included with the repo:
  - Configures the console output to be USB Serial/JTAG

## Example Output

Individual frame output:

```
Timestamp: 5, ID: 000, Data Len: 8, Data: 00 01 02 03 04 05 06 07
Timestamp: 6, ID: 001, Data Len: 8, Data: 00 01 02 03 04 05 06 07
Timestamp: 7, ID: 002, Data Len: 8, Data: 00 01 02 03 04 05 06 07
Timestamp: 8, ID: 003, Data Len: 8, Data: 00 01 02 03 04 05 06 07
Timestamp: 8, ID: 004, Data Len: 8, Data: 00 01 02 03 04 05 06 07
Timestamp: 9, ID: 005, Data Len: 8, Data: 00 01 02 03 04 05 06 07
Timestamp: 10, ID: 006, Data Len: 8, Data: 00 01 02 03 04 05 06 07
Timestamp: 11, ID: 007, Data Len: 8, Data: 00 01 02 03 04 05 06 07
Timestamp: 11, ID: 000, Data Len: 8, Data: 00 01 02 03 0F 05 06 07
Timestamp: 13, ID: 002, Data Len: 8, Data: 00 01 02 03 0F 05 06 07
Timestamp: 14, ID: 004, Data Len: 8, Data: 00 01 02 03 0F 05 06 07
Timestamp: 15, ID: 006, Data Len: 8, Data: 00 01 02 03 0F 05 06 07
```

CAN frame table output:

```
|-------------------------------------------------------------------------------------------------|
| Time Since Last Change (ms) | ID  | DLC | Data                      | Previous Data             |
|-------------------------------------------------------------------------------------------------|
| 5001                        | 000 |  8  | 00 01 02 03 0F 05 06 07   | 00 01 02 03 04 05 06 07   |
|-------------------------------------------------------------------------------------------------|
| 5004                        | 001 |  8  | 00 01 02 03 04 05 06 07   |                           |
|-------------------------------------------------------------------------------------------------|
| 5000                        | 002 |  8  | 00 01 02 03 0F 05 06 07   | 00 01 02 03 04 05 06 07   |
|-------------------------------------------------------------------------------------------------|
| 5003                        | 003 |  8  | 00 01 02 03 04 05 06 07   |                           |
|-------------------------------------------------------------------------------------------------|
| 5000                        | 004 |  8  | 00 01 02 03 0F 05 06 07   | 00 01 02 03 04 05 06 07   |
|-------------------------------------------------------------------------------------------------|
| 5002                        | 005 |  8  | 00 01 02 03 04 05 06 07   |                           |
|-------------------------------------------------------------------------------------------------|
| 4999                        | 006 |  8  | 00 01 02 03 0F 05 06 07   | 00 01 02 03 04 05 06 07   |
|-------------------------------------------------------------------------------------------------|
| 5001                        | 007 |  8  | 00 01 02 03 04 05 06 07   |                           |
|-------------------------------------------------------------------------------------------------|
  Current Dropped Frames: 0
```

## Current Limitations

- Self-test loopback only
- Only configured for standard CAN IDs
- No filtering or logging of frames

## Next Steps

- Tap into the car's (BMW E87) PT-CAN at the FRM module via the transceiver in listen only mode.
- Filtering and outputting relevant frames to the console
