//
// Created by elmer on 4.10.2024.
//

#ifndef KEIJOS_RETIREMENT_PLAN_PRODUCT_LOGGER_HPP
#define KEIJOS_RETIREMENT_PLAN_PRODUCT_LOGGER_HPP

#include "register_handler.h"
#include "task_defines.h"
#include <memory>
#include <cstring>
#include <cstdint>

class Logger {
  public:
    Logger(shared_i2c i2c_0);
    QueueHandle_t get_reading_queue_handle(void);
    static void read_eeprom_task(void *param);
    float co2_target_return;
    uint16_t fan_speed_return;
 private:
    std::shared_ptr<PicoI2C> i2c;
    QueueHandle_t reading_queue;
    SemaphoreHandle_t mutex;
    static void write_to_eeprom_task(void *pvParams);
    void write_address(uint16_t address);
    void write_byte(uint16_t address, char c);
    void write_page(uint16_t address, uint8_t *src, size_t size);
    void read_page(uint16_t address, uint8_t *dst, size_t size);
    void storeData(int co2_target, int fan_speed);
    void fetchData(int *co2_target, int *fan_speed, int *arr);
};

uint16_t crc16(const uint8_t *data, size_t length);
void appendCrcToBase8Array(uint8_t *base8Array, int *arrayLen);
int createCredentialArray(std::string str, uint8_t *arr);
int getChecksum(uint8_t *base8Array, int base8ArrayLen);
bool verifyDataIntegrity(uint8_t *base8Array, int base8ArrayLen);

#endif // KEIJOS_RETIREMENT_PLAN_PRODUCT_LOGGER_HPP
