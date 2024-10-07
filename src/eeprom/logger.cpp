//
// Created by elmer on 4.10.2024.
//

#include "logger.hpp"
#include <iostream>

Logger::Logger(shared_i2c i2c_0) {
    this->i2c = i2c_0;
    co2_target_return_val = 0;
    fan_speed_return_val = 0;
    mode_return_val = 0;
    reading_queue = xQueueCreate(10, sizeof(Reading));
    xTaskCreate(eeprom_task, "write to eeprom", 1024, this, TaskPriority::LOW, NULL);
}

QueueHandle_t Logger::get_reading_queue_handle(void) { return reading_queue; }

void Logger::eeprom_task(void *pvParameters) {
    auto logger = static_cast<Logger *>(pvParameters);

    int co2_tar_read = 0, fan_speed_read = 0, mode_read = 0;
    int arr[3] = {0, 0, 0};
    logger->fetchData(&co2_tar_read, &fan_speed_read, &mode_read, arr);

    logger->co2_target_return_val = co2_tar_read;
    logger->fan_speed_return_val = fan_speed_read;
    logger->mode_return_val = mode_read;

    logger->is_done = true;

    int co2_target = 0;
    int fan_speed = 0;
    int mode = 0;

    Reading reading;

    for (;;) {
        xQueueReceive(logger->reading_queue, &reading, portMAX_DELAY);
        switch (reading.type) {
            case ReadingType::CO2_TARGET:
                co2_target = reading.value.f32;
                break;
            case ReadingType::FAN_SPEED:
                fan_speed = reading.value.u16;
                break;
            case ReadingType::MODE:
                mode = reading.value.u16;
                break;
            default:
                break;
        }
        logger->storeData(co2_target, fan_speed, mode);
    }
}

int Logger::return_c02_target() { return this->co2_target_return_val; }

int Logger::return_fan_speed() { return this->fan_speed_return_val; }

int Logger::return_mode() { return this->mode_return_val; }

void Logger::write_address(uint16_t address) {
    uint8_t out[2] = {(uint8_t)(address >> 8), (uint8_t)address};
    i2c->write(0x50, out, 2);
}

void Logger::write_page(uint16_t address, uint8_t *src, size_t size) {
    uint8_t out[size + 2];
    out[0] = address >> 8; // Upper bits of the address
    out[1] = address;      // Lower bits of the address

    for (size_t i = 0; i < size; i++) {
        out[i + 2] = src[i];
    }
    i2c->write(0x50, out, size + 2);
}

void Logger::write_byte(uint16_t address, char c) {
    uint8_t out[3] = {(uint8_t)(address >> 8), (uint8_t)address, c};

    i2c->write(0x50, out, 3);
}

void Logger::read_page(uint16_t address, uint8_t *dst, size_t size) {
    write_address(address);

    i2c->read(0x50, dst, size);
}

void Logger::storeData(int co2_target, int fan_speed, int mode) {
    char co2_target_str[64], fan_speed_str[64], mode_target_str[64];
    snprintf(co2_target_str, sizeof(co2_target_str), "%d", co2_target);
    snprintf(fan_speed_str, sizeof(fan_speed_str), "%d", fan_speed);
    snprintf(mode_target_str, sizeof(mode_target_str), "%d", mode);

    uint8_t co2_target_Arr[64];
    uint8_t fan_speed_Arr[64];
    uint8_t mode_Arr[64];

    int co2_target_Len = createCredentialArray(co2_target_str, co2_target_Arr);
    int fan_speed_Len = createCredentialArray(fan_speed_str, fan_speed_Arr);
    int mode_Len = createCredentialArray(mode_target_str, mode_Arr);

    appendCrcToBase8Array(co2_target_Arr, &co2_target_Len);
    appendCrcToBase8Array(fan_speed_Arr, &fan_speed_Len);
    appendCrcToBase8Array(mode_Arr, &mode_Len);

    write_page(0, co2_target_Arr, co2_target_Len);
    vTaskDelay(pdMS_TO_TICKS(10));
    write_page(64, fan_speed_Arr, fan_speed_Len);
    vTaskDelay(pdMS_TO_TICKS(10));
    write_page(128, mode_Arr, mode_Len);
    vTaskDelay(pdMS_TO_TICKS(10));

    return;
}

void Logger::fetchData(int *co2_target, int *fan_speed, int *mode, int *arr) {
    uint8_t co2_target_Arr[64] = {0};
    uint8_t fan_speed_Arr[64] = {0};
    uint8_t mode_Arr[64] = {0};

    for (int i = 0; i < 3; ++i) {
        arr[i] = 0;
    }

    read_page(0, co2_target_Arr, 64);
    vTaskDelay(pdMS_TO_TICKS(10));
    read_page(64, fan_speed_Arr, 64);
    vTaskDelay(pdMS_TO_TICKS(10));
    read_page(128, mode_Arr, 64);
    vTaskDelay(pdMS_TO_TICKS(10));

    if (verifyDataIntegrity(co2_target_Arr, (int)co2_target_Arr[1])) {
        char array[10];
        memcpy(array, co2_target_Arr + 2, (int)co2_target_Arr[1] - 3);
        std::size_t len = (int)co2_target_Arr[1] - 3;
        *co2_target = std::stoi(array, &len);
        arr[0] = 1;
    }
    if (verifyDataIntegrity(fan_speed_Arr, (int)fan_speed_Arr[1])) {
        char array[10];
        memcpy(array, fan_speed_Arr + 2, (int)fan_speed_Arr[1] - 3);
        std::size_t len = (int)fan_speed_Arr[1] - 3;
        *fan_speed = std::stoi(array, &len);
        arr[1] = 1;
    }

    if (verifyDataIntegrity(mode_Arr, (int)mode_Arr[1])) {
        char array[10];
        memcpy(array, mode_Arr + 2, (int)mode_Arr[1] - 3);
        std::size_t len = (int)mode_Arr[1] - 3;
        *mode = std::stoi(array, &len);
        arr[2] = 1;
    }
    return;
}

uint16_t crc16(const uint8_t *data, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data++;
        x ^= x >> 4;
        crc =
            (crc << 8) ^ (static_cast<uint16_t>(x << 12)) ^ (static_cast<uint16_t>(x << 5)) ^ static_cast<uint16_t>(x);
    }

    return crc;
}

void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen) {
    uint16_t crc = crc16(base8Array, *arrayLen);
    base8Array[*arrayLen] = crc >> 8;       // MSB
    base8Array[*arrayLen + 1] = crc & 0xFF; // LSB
    *arrayLen += 2;                         // Update the array length to reflect the addition of the CRC
}

int createCredentialArray(std::string str, uint8_t *arr) {
    int length = str.length();
    if (length > 60) { return -1; }
    arr[0] = 1;
    arr[1] = length + 5; // Length of the string + 2 bytes for the CRC + 2 bytes for the length.
    memcpy(arr + 2, str.c_str(), length + 1);
    return length + 3;
}

int getChecksum(uint8_t *base8Array, int base8ArrayLen) {
    uint16_t crc = crc16(base8Array, base8ArrayLen);
    return crc;
}

bool verifyDataIntegrity(uint8_t *base8Array, int base8ArrayLen) {
    if (getChecksum(base8Array, base8ArrayLen) == 0) { return true; }
    return false;
}
