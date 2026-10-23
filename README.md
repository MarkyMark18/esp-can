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
- A FreeRTOS task then retrieves frames from the queue and prints their ID (hex), DLC (int) and data (hex) (length based on the DLC) to the monitor
- Test function transmit_test_twai_frames() takes a number of frames as a parameter and transmits these
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

```
ID: 3E5, Data Len: 0, Data:
ID: 3E6, Data Len: 1, Data: E6
ID: 3E7, Data Len: 2, Data: E7 E8
ID: 3E8, Data Len: 3, Data: E8 E9 EA
ID: 3E9, Data Len: 4, Data: E9 EA EB EC
ID: 3EA, Data Len: 5, Data: EA EB EC ED EE
ID: 3EB, Data Len: 6, Data: EB EC ED EE EF F0
ID: 3EC, Data Len: 7, Data: EC ED EE EF F0 F1 F2
ID: 3ED, Data Len: 8, Data: ED EE EF F0 F1 F2 F3 F4
```

## Current Limitations

- Self-test loopback only
- Only configured for standard CAN IDs 
- No filtering or logging of frames

## Next Steps

- Tap into the car's (BMW E87) PT-CAN at the FRM module via the transceiver in listen only mode. 
- Filtering and outputting relevant frames to the console
- 