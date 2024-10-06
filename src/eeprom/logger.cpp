//
// Created by elmer on 4.10.2024.
//

#include "logger.hpp"
#include <iostream>

Logger::Logger(shared_i2c i2c_0) {
    this->i2c = i2c_0;
    co2_target = 0;
    fan_speed = 0;
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
    float aa;
    uint16_t gg;
    int arr[2] = {0, 0};

    if (xSemaphoreTake(logger->mutex, portMAX_DELAY) == pdTRUE) {
        logger->fetchData(&aa, &gg, arr);

        logger->co2_target = aa;
        logger->fan_speed = gg;
        xSemaphoreGive(logger->mutex);
    }

    std::cout << "CO2 target " << logger->co2_target << std::endl;
    std::cout << "fan speed " << logger->fan_speed << std::endl;
    vTaskDelete(nullptr);
}

void Logger::write_to_eeprom_task(void *pvParameters){
    auto logger = static_cast<Logger *>(pvParameters);
    vTaskDelay(10000);

    float co2_target = 0.0;
    uint16_t fan_speed = 0;
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

                std::cout << "CO2 " << co2_target << std::endl;
                std::cout << "temperature " << fan_speed << std::endl;

                logger->storeData(&co2_target, &fan_speed);

                std::cout << "done " << std::endl;
            }
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

    i2c->write(0x50, out, 2);
}

void Logger::write_byte(uint16_t address, char c) {
    uint8_t out[3] = {(uint8_t)(address >> 8), (uint8_t)address, c};

    i2c->write(0x50, out, 3);
}

void Logger::read_page(uint16_t address, uint8_t *dst, size_t size) {
    write_address(address);

    i2c->read(0x50, dst, size);
}

void Logger::storeData(float* co2_target, uint16_t * fan_speed){
    char co2_targ_Str[20], fan_speed_Str[20];
    snprintf(co2_targ_Str, sizeof(co2_targ_Str), "%f", co2_target);
    snprintf(fan_speed_Str, sizeof(fan_speed_Str), "%u", fan_speed);

    uint8_t co2_target_Arr[64];
    uint8_t fan_speed_Arr[64];

    int co2_target_Len = createCredentialArray(co2_targ_Str, co2_target_Arr);
    int fan_speed_Len = createCredentialArray(fan_speed_Str, fan_speed_Arr);

    appendCrcToBase8Array(co2_target_Arr, &co2_target_Len);
    appendCrcToBase8Array(fan_speed_Arr, &fan_speed_Len);

    //TODO halutaan tallentaa co2 target, fanspeed ja mode
    write_page(4160 + (64 * 0), co2_target_Arr, co2_target_Len);
    write_page(4160 + (64 * 1), fan_speed_Arr, fan_speed_Len);

    return;
}

void Logger::fetchData(float *co2_target, uint16_t *fan_speed, int *arr){
    uint8_t co2_target_Arr[64];
    uint8_t fan_speed_Arr[64];

    for (int i = 0; i < 2; ++i) {
        arr[i] = 0;
    }

    read_page(4160 + (64 * 0), co2_target_Arr, 64);
    read_page(4160 + (64 * 1), fan_speed_Arr, 64);

    char buffer[20]; // Temporary buffer to store string representation of float

    // Check and convert co2_target_Arr to float co2 target
    if (verifyDataIntegrity(co2_target_Arr, (int)co2_target_Arr[1])) {
        memcpy(buffer, co2_target_Arr + 2, (int)co2_target_Arr[1] - 3);
        buffer[(int)co2_target_Arr[1] - 3] = '\0'; // Null-terminate string
        *co2_target = std::strtof(buffer, nullptr);
        arr[0] = 1;
    }

    // Check and convert fan_speed_Arr to uint16_t fan_speed
    if (verifyDataIntegrity(fan_speed_Arr, (int)fan_speed_Arr[1])) {
        memcpy(buffer, fan_speed_Arr + 2, (int)fan_speed_Arr[1] - 3);
        buffer[(int)fan_speed_Arr[1] - 3] = '\0'; // Null-terminate string

        unsigned long fan_speed_value = std::strtoul(buffer, nullptr, 10);

        if (fan_speed_value <= UINT16_MAX) {
            *fan_speed = (uint16_t)fan_speed_value;  // Cast to uint16_t
        } else {
            std::cout << "fuq" << std::endl;
        }
        arr[1] = 1;
    }

    return;
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