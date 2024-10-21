#include <iostream>  // For C++ standard libraries
#include <cstddef>
#include <string.h>

extern "C" {
    #include "driver/twai.h"  // TWAI (CAN) driver
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

// Define CAN TX and RX pins
#define CAN_TX 5
#define CAN_RX 4

// Function prototypes
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

    uint8_t receivedData[16] = {0};  // Buffer for received data
    uint8_t receivedPart1[8] = {0};
    uint8_t receivedPart2[8] = {0};

    while (1) {
        twai_message_t rxMsg;

        // Receive the first CAN message (Part 1)
        if (twai_receive(&rxMsg, pdMS_TO_TICKS(1000)) == ESP_OK && rxMsg.identifier == 0x200) {
            // Copy the data from the CAN message to the received buffer
            for (int i = 0; i < 8; i++) {
                receivedPart1[i] = rxMsg.data[i];
            }
            logCanMessage(rxMsg, "Received CANMsg Part 1", "Rx");
        } else {
            std::cout << "No message or failed to receive CANMsg Part 1" << std::endl;
        }

        // Receive the second CAN message (Part 2)
        if (twai_receive(&rxMsg, pdMS_TO_TICKS(1000)) == ESP_OK && rxMsg.identifier == 0x201) {
            // Copy the data from the CAN message to the received buffer
            for (int i = 0; i < 8; i++) {
                receivedPart2[i] = rxMsg.data[i];
            }
            logCanMessage(rxMsg, "Received CANMsg Part 2", "Rx");
        } else {
            std::cout << "No message or failed to receive CANMsg Part 2" << std::endl;
        }

        // Combine the two parts into one buffer
        memcpy(receivedData, receivedPart1, 8);
        memcpy(receivedData + 8, receivedPart2, 8);

        // Extract RPM and Speed from the received data
        uint16_t receivedRpm = (receivedData[0] << 8) | receivedData[1];  // Combine two bytes into 16-bit RPM value
        uint16_t receivedSpeed = (receivedData[2] << 8) | receivedData[3];  // Combine two bytes into 16-bit Speed value

        // Log the extracted data
        std::cout << std::dec << "Received RPM: " << receivedRpm << std::endl;
        std::cout << std::dec << "Received Speed: " << receivedSpeed << " km/h" << std::endl;

        vTaskDelay(pdMS_TO_TICKS(1000));  // Wait for 1 second before receiving again
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
