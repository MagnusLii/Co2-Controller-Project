//
// Created by elmer on 4.10.2024.
//

#include "logger.hpp"
#include <iostream>

Logger::Logger(shared_i2c i2c_0) {
    this->i2c = i2c_0;
    co2_target_return = 0;
    fan_speed_return = 0;
    mutex = xSemaphoreCreateMutex();
    reading_queue = xQueueCreate(20, sizeof(Reading));
    xTaskCreate(read_eeprom_task, "read from eeprom", 256, this, TaskPriority::ABSOLUTE, NULL);
    xTaskCreate(write_to_eeprom_task, "write to eeprom", 256, this, TaskPriority::LOW, NULL);
}

QueueHandle_t Logger::get_reading_queue_handle(void) {
    return reading_queue;
}

void Logger::read_eeprom_task(void *pvParameters) {
    auto logger = static_cast<Logger *>(pvParameters);
    int aa = 0, gg = 0;
    int arr[2] = {0, 0};

    if (xSemaphoreTake(logger->mutex, portMAX_DELAY) == pdTRUE) {
        logger->fetchData(&aa, &gg, arr);

        logger->co2_target_return = aa;
        logger->fan_speed_return = gg;
        xSemaphoreGive(logger->mutex);
    }
    vTaskDelete(nullptr);
}

void Logger::write_to_eeprom_task(void *pvParameters){
    auto logger = static_cast<Logger *>(pvParameters);

    int co2_target = 0;
    int fan_speed = 0;
    Reading reading;
    for(;;){
        if (xSemaphoreTake(logger->mutex, portMAX_DELAY) == pdTRUE) {
            while (xQueueReceive(logger->reading_queue, &reading, 0) == pdTRUE){
                switch (reading.type) {
                    case ReadingType::CO2:
                        co2_target = reading.value.f32;
                        break;
                    case ReadingType::FAN_SPEED:
                        fan_speed = reading.value.u16;
                        break;
                    default:
                        break;
                }
            }
        logger->storeData(co2_target, fan_speed);
        xSemaphoreGive(logger->mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void Logger::write_address(uint16_t address) {
    uint8_t out[2] = {(uint8_t)(address >> 8), (uint8_t)address};
    i2c->write(0x50, out, 2);
}

void Logger::write_page(uint16_t address, uint8_t *src, size_t size) {
    uint8_t out[size + 2];
    out[0] = address >> 8; // Upper bits of the address
    out[1] = address; // Lower bits of the address

    for (int i = 0; i < size; i++) {
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

void Logger::storeData(int co2_target, int fan_speed){
    char co2_target_str[10], fan_speed_str[10];
    snprintf(co2_target_str, sizeof(co2_target_str), "%d", co2_target);
    snprintf(fan_speed_str, sizeof(fan_speed_str), "%d", fan_speed);

    uint8_t co2_target_Arr[64];
    uint8_t fan_speed_Arr[64];

    int co2_target_Len = createCredentialArray(co2_target_str, co2_target_Arr);
    int fan_speed_Len = createCredentialArray(fan_speed_str, fan_speed_Arr);

    appendCrcToBase8Array(co2_target_Arr, &co2_target_Len);
    appendCrcToBase8Array(fan_speed_Arr, &fan_speed_Len);

    write_page(0, co2_target_Arr, co2_target_Len);
    write_page(64, fan_speed_Arr, fan_speed_Len);

    return;
}

void Logger::fetchData(int *co2_target, int *fan_speed, int *arr){
    uint8_t co2_target_Arr[64];
    uint8_t fan_speed_Arr[64];

    for (int i = 0; i < 2; ++i) {
        arr[i] = 0;
    }

    read_page(0, co2_target_Arr, 64);
    read_page(64, fan_speed_Arr, 64);

    char buffer[20]; // Temporary buffer to store string representation of float

    // Check and convert co2_target_Arr to float co2 target
    if (verifyDataIntegrity(co2_target_Arr, (int)co2_target_Arr[1])) {
        char array[10];
        memcpy(array, co2_target_Arr + 2, (int)co2_target_Arr[1] - 3);
        std::size_t len = (int)co2_target_Arr[1] - 3;
        *co2_target = std::stoi(array, &len);
        arr[1] = 1;
    }
    // Check and convert fan_speed_Arr to uint16_t fan_speed
    if (verifyDataIntegrity(fan_speed_Arr, (int)fan_speed_Arr[1])) {
        char array[10];
        memcpy(array, fan_speed_Arr + 2, (int)fan_speed_Arr[1] - 3);
        std::size_t len = (int)fan_speed_Arr[1] - 3;
        *fan_speed = std::stoi(array, &len);
        arr[1] = 1;
    }
    return;
}

uint16_t crc16(const uint8_t *data, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ (static_cast<uint16_t>(x << 12)) ^ (static_cast<uint16_t>(x << 5)) ^ static_cast<uint16_t>(x);
    }

    return crc;
}

void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen){
    uint16_t crc = crc16(base8Array, *arrayLen);
    base8Array[*arrayLen] = crc >> 8;       // MSB
    base8Array[*arrayLen + 1] = crc & 0xFF; // LSB
    *arrayLen += 2; // Update the array length to reflect the addition of the CRC
}

int createCredentialArray(std::string str, uint8_t *arr){
    int length = str.length();
    if (length > 60){
        return -1;
    }
    arr[0] =  1;
    arr[1] = length + 5; // Length of the string + 2 bytes for the CRC + 2 bytes for the length.
    memcpy(arr + 2, str.c_str(), length + 1);
    return length + 3;
}

int getChecksum(uint8_t *base8Array, int base8ArrayLen) {
    uint16_t crc = crc16(base8Array, base8ArrayLen);
    return crc;
}

bool verifyDataIntegrity(uint8_t *base8Array, int base8ArrayLen) {
    if (getChecksum(base8Array, base8ArrayLen) == 0) {return true;}
    return false;
}
