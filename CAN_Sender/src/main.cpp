#include <iostream>
#include <cstddef>

extern "C" {
    #include "driver/twai.h"  // TWAI (CAN) driver
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

// Define CAN TX and RX pins
#define CAN_TX 5
#define CAN_RX 4

// Function prototypes
void sendCANMessage(uint16_t canId, uint8_t* data, const char* messageName);
void logCanMessage(twai_message_t frame, const char* messageName, const char* direction);

extern "C" void app_main(void) {
    // Initialize serial communication for debugging
    std::cout << "Initializing CAN bus..." << std::endl;

    // TWAI configuration: use 500 kbps bitrate
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        std::cout << "TWAI driver installed" << std::endl;
    } else {
        std::cout << "Failed to install TWAI driver" << std::endl;
        return;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        std::cout << "TWAI driver started successfully!" << std::endl;
    } else {
        std::cout << "Failed to start TWAI driver" << std::endl;
        return;
    }

    while (1) {
        // Example data: RPM and Speed
        uint16_t rpm = 3000;  // Example RPM value
        uint16_t speed = 80;  // Example speed value in km/h

        // Prepare data block (16 bytes)
        uint8_t plainText[16] = {0};
        plainText[0] = (rpm >> 8) & 0xFF;  // High byte of RPM
        plainText[1] = rpm & 0xFF;         // Low byte of RPM
        plainText[2] = (speed >> 8) & 0xFF;  // High byte of Speed
        plainText[3] = speed & 0xFF;         // Low byte of Speed

        // Send the data as two CAN messages (8 bytes each)
        sendCANMessage(0x200, plainText, "CANMsg Part 1");
        sendCANMessage(0x201, plainText + 8, "CANMsg Part 2");

        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay between messages
    }
}

/*void sendCANMessage(uint16_t canId, uint8_t* data, const char* messageName) {
    twai_message_t testFrame = {0};
    testFrame.identifier = canId;  // Assign the CAN ID
    testFrame.extd = 0;  // Standard frame (11-bit identifier)
    testFrame.data_length_code = 8;  // DLC: 8 bytes of data

    // Copy 8 bytes of data into the frame's data buffer
    for (int i = 0; i < 8; i++) {
        testFrame.data[i] = data[i];
    }

    // Transmit the CAN message
    if (twai_transmit(&testFrame, pdMS_TO_TICKS(1000)) == ESP_OK) {
        logCanMessage(testFrame, messageName, "Sent");
    } else {
        logCanMessage(testFrame, messageName, "Failed");
    }
}
*/
void sendCANMessage(uint16_t canId, uint8_t* data, const char* messageName) {
    // Explicitly initialize the necessary fields of twai_message_t
    twai_message_t testFrame;
    testFrame.identifier = canId;         // Assign the CAN ID
    testFrame.extd = 0;                   // Standard frame (11-bit identifier)
    testFrame.rtr = 0;                    // No Remote Transmission Request (RTR)
    testFrame.ss = 1;                     // Single Shot transmission
    testFrame.dlc_non_comp = 0;           // Not using DLC non-compliant mode
    testFrame.data_length_code = 8;       // DLC: 8 bytes of data

    // Copy 8 bytes of data into the frame's data buffer
    for (int i = 0; i < 8; i++) {
        testFrame.data[i] = data[i];
    }

    // Transmit the CAN message
    if (twai_transmit(&testFrame, pdMS_TO_TICKS(1000)) == ESP_OK) {
        logCanMessage(testFrame, messageName, "Sent");
    } else {
        logCanMessage(testFrame, messageName, "Failed");
    }
}


void logCanMessage(twai_message_t frame, const char* messageName, const char* direction) {
    unsigned long currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unsigned long seconds = (currentTime / 1000) % 60;
    unsigned long minutes = (currentTime / (1000 * 60)) % 60;
    unsigned long hours = (currentTime / (1000 * 60 * 60)) % 24;
    unsigned long milliseconds = currentTime % 1000;

    std::cout << hours << ":" << minutes << ":" << seconds << ":" << milliseconds << "    "
              << direction << "    0x" << std::hex << frame.identifier << "    "
              << messageName << "    " << frame.data_length_code << "    ";

    // Print each data byte in hex format
    for (int i = 0; i < frame.data_length_code; i++) {
        std::cout << std::hex << (int)frame.data[i] << " ";
    }
    std::cout << std::endl;
}
