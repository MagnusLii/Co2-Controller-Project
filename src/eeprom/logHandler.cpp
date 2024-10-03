#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include "eeprom.h"
#include "logHandler.h"
/*#include "commhandler.h"
#include "debugprint.h"*/
#include <memory.h>

#define CRC_LEN 2
#define LOG_LEN 6                     // Does not include CRC
#define LOG_ARR_LEN (LOG_LEN + CRC_LEN) // Includes CRC

#define DISPENSER_STATE_LEN 6                                 // Does not include CRC
#define DISPENSER_STATE_ARR_LEN (DISPENSER_STATE_LEN + CRC_LEN) // Includes CRC

#define LOG_START_ADDR 0
#define LOG_END_ADDR (LOG_START_ADDR + (MAX_LOGS * LOG_SIZE))  // The final address is never actually used due the starting from 0. Acts as a separator for the next log type
#define LOG_SIZE 8
#define MAX_LOGS 256

#define REBOOT_STATUS_START_ADDR (LOG_END_ADDR + LOG_SIZE)
#define REBOOT_STATUS_END_ADDR (REBOOT_STATUS_START_ADDR + (MAX_LOGS * LOG_SIZE))

#define DATA_ARR_SIZE 64
#define CREDENTIALS_STORAGE_BLOCK 256 
#define CREDENTIALS_START_ADDR (REBOOT_STATUS_END_ADDR + (DATA_ARR_SIZE - (REBOOT_STATUS_END_ADDR %                                                    \
                       DATA_ARR_SIZE))) // Aligns the address to the next 64 byte boundary.
#define CREDENTIALS_NUM 4
#define CREDENTIALS_END_ADDR (CREDENTIALS_START_ADDR + (CREDENTIALS_STORAGE_BLOCK * (CREDENTIALS_NUM - 1)))

#define CREDENTIALS_USE_ADDR 4160

// TODO: Create better messages
const char *logMessages[] = {
    "Test",
    "Test2",
};

// TODO: Create better messages
const char *rebootStatusMessages[] = {
    "OK",
    "Crash",
    "Forced Reboot"
};

// TODO: Delete this.
void LogHandler::printPrivates() {
    std::cout << "Log Address: " << this->unusedLogAddr << std::endl;
    std::cout << "Reboot Status Address: " << this->unusedRebootStatusAddr << std::endl;
    std::cout << "Unused Comm Config Address: " << this->unusedCommConfigAddr << std::endl;
    std::cout << "Current Comm Config Address: " << this->currentCommConfigAddr << std::endl;
}

void LogHandler::incrementUnusedLogIndex(const LogType logType) {
    if (logType == LOGTYPE_MSG_LOG){
        this->unusedLogAddr += LOG_SIZE;
        if (this->unusedLogAddr >= LOG_END_ADDR){
            this->clearAllLogs(LOGTYPE_MSG_LOG);
        }
    }
    else if (logType == LOGTYPE_REBOOT_STATUS){
        this->unusedRebootStatusAddr += LOG_SIZE;
        if (this->unusedRebootStatusAddr >= REBOOT_STATUS_END_ADDR){
            this->clearAllLogs(LOGTYPE_REBOOT_STATUS);
        }
    }
    else if (logType == LOGTYPE_COMM_CONFIG){
        this->unusedCommConfigAddr = CREDENTIALS_USE_ADDR;
        this->currentCommConfigAddr = CREDENTIALS_START_ADDR;
        
        /*
        eeprom_write_byte(this->currentCommConfigAddr, 0);
        this->currentCommConfigAddr = unusedCommConfigAddr;
        this->unusedCommConfigAddr += CREDENTIALS_STORAGE_BLOCK;
        if (this->unusedCommConfigAddr >= CREDENTIALS_END_ADDR){
            this->unusedCommConfigAddr = CREDENTIALS_START_ADDR;
        }
        */
    }
}

void LogHandler::pushLog(LogMessage messageCode){
    uint8_t logArray[LOG_LEN];
    int logLen = LOG_LEN;
    uint32_t timestamp = getTimestampSinceBoot(this->bootTimestamp);
    createLogArray(logArray, messageCode, timestamp);
    LogHandler::enterLogToEeprom(logArray, &logLen, this->unusedLogAddr);
    LogHandler::incrementUnusedLogIndex(LOGTYPE_MSG_LOG);

    // string editing.
    std::string log = logMessages[messageCode];
    std::string message = "\"{\"log\":\"" + log + "\"}\"";
    //(message);
    
    //this->commHandler->publish(TopicType::LOG_SEND, message.c_str());
}

void LogHandler::pushRebootLog(RebootStatusCodes statusCode){
    uint8_t logArray[LOG_LEN];
    int logLen = LOG_LEN;
    uint32_t timestamp = getTimestampSinceBoot(this->bootTimestamp);
    createLogArray(logArray, statusCode, timestamp);
    LogHandler::enterLogToEeprom(logArray, &logLen, this->unusedRebootStatusAddr);
    LogHandler::incrementUnusedLogIndex(LOGTYPE_REBOOT_STATUS);
    
    // string editing.
    std::string log = rebootStatusMessages[statusCode];
    std::string message = "\"{\"devStatus\":\"" + log + "\"}\"";
    //DPRINT(message);
    
    //this->commHandler->publish(TopicType::STATUS_SEND, message.c_str());
}

void LogHandler::clearAllLogs(const LogType logType){
    uint16_t logAddr = 0;

    if (logType == LOGTYPE_MSG_LOG){
        logAddr = LOG_START_ADDR;
        for (int i = 0; i < MAX_LOGS; i++){
            eeprom_write_byte(logAddr, 0);
            logAddr += LOG_SIZE;
        }
        this->unusedLogAddr = LOG_START_ADDR;
    }
    else if (logType == LOGTYPE_REBOOT_STATUS){
        logAddr = REBOOT_STATUS_START_ADDR;
        for (int i = 0; i < MAX_LOGS; i++){
            eeprom_write_byte(logAddr, 0);
            logAddr += LOG_SIZE;
        }
        this->unusedRebootStatusAddr = REBOOT_STATUS_START_ADDR;
    }
    return;
}

void LogHandler::findFirstAvailableLog(const LogType logType){
    uint16_t logAddr = 0;

    switch (logType){
    case LOGTYPE_MSG_LOG:
        logAddr = LOG_START_ADDR;
        for (int i = 0; i < MAX_LOGS; i++){
            if ((int)eeprom_read_byte(logAddr) == 0){
                this->unusedLogAddr = logAddr;
                return;
            }
            logAddr += LOG_SIZE;
        }

        LogHandler::clearAllLogs(LOGTYPE_MSG_LOG);
        this->unusedLogAddr = LOG_START_ADDR;

        break;
    case LOGTYPE_REBOOT_STATUS:
        logAddr = REBOOT_STATUS_START_ADDR;
        for (int i = 0; i < MAX_LOGS; i++){
            if ((int)eeprom_read_byte(logAddr) == 0){
                this->unusedRebootStatusAddr = logAddr;
                return;
            }
            logAddr += LOG_SIZE;
        }

        LogHandler::clearAllLogs(LOGTYPE_REBOOT_STATUS);
        this->unusedRebootStatusAddr = REBOOT_STATUS_START_ADDR;

        break;

    //TODO: verify this
    case LOGTYPE_COMM_CONFIG:
        this->unusedCommConfigAddr = CREDENTIALS_USE_ADDR;
        this->currentCommConfigAddr = CREDENTIALS_START_ADDR;

    /*
        logAddr = CREDENTIALS_START_ADDR;
        for (int i = 0; i < CREDENTIALS_NUM; i++){
            std::cout << "logAddr test: " << logAddr << std::endl;
            if ((int)eeprom_read_byte(logAddr) == 0){
                this->unusedCommConfigAddr = logAddr;
                this->currentCommConfigAddr = logAddr - CREDENTIALS_STORAGE_BLOCK;

                if (logAddr == CREDENTIALS_START_ADDR){ // if unusedCommConfigAddr == CREDENTIALS_START_ADDR, then the currentCommConfigAddr is CREDENTIALS_END_ADDR.
                this->currentCommConfigAddr = CREDENTIALS_END_ADDR;
                }
                std::cout << "Unused Comm Config Addr: " << this->unusedCommConfigAddr << std::endl;
                std::cout << "Current Comm Config Addr: " << this->currentCommConfigAddr << std::endl;
                return;
            }
            logAddr += CREDENTIALS_STORAGE_BLOCK;
        }
        this->unusedCommConfigAddr = CREDENTIALS_START_ADDR;
        this->currentCommConfigAddr = CREDENTIALS_END_ADDR;
        std::cout << "Unused Comm Config Addr2: " << this->unusedCommConfigAddr << std::endl;
        std::cout << "Current Comm Config Addr2: " << this->currentCommConfigAddr << std::endl;
        break;
    */
    }
    return;
}

void LogHandler::enterLogToEeprom(uint8_t *base8Array, int *arrayLen, const int logAddr) {
    uint8_t crcAppendedArray[*arrayLen + CRC_LEN];
    memcpy(crcAppendedArray, base8Array, *arrayLen);
    appendCrcToBase8Array(crcAppendedArray, arrayLen);
    eeprom_write_page(logAddr, crcAppendedArray, *arrayLen);
}

void LogHandler::createLogArray(uint8_t *array, int messageCode, uint32_t timestamp){
    array[LOG_USE_STATUS] = LOG_IN_USE;
    array[MESSAGE_CODE] = messageCode;

    // TODO: redo replace timestamp with realtime date once RTC is implemented.
    array[TIMESTAMP_LSB] = (uint8_t)(timestamp & 0xFF);         // LSB of timestamp
    array[TIMESTAMP_MSB2] = (uint8_t)((timestamp >> 8) & 0xFF);
    array[TIMESTAMP_MSB1] = (uint8_t)((timestamp >> 16) & 0xFF);
    array[TIMESTAMP_MSB] = (uint8_t)((timestamp >> 24) & 0xFF); // MSB of timestamp

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
    *arrayLen += CRC_LEN; // Update the array length to reflect the addition of the CRC
}

int getChecksum(uint8_t *base8Array, int base8ArrayLen) {
    uint16_t crc = crc16(base8Array, base8ArrayLen);
    // TODO: REMOVE TROUBLESHOOT CODE
    /*
    std::cout << "CRC: " << crc << std::endl;
    */
    return crc;
}

bool verifyDataIntegrity(uint8_t *base8Array, int base8ArrayLen) {
    // TODO: REMOVE TROUBLESHOOT CODE
    for (int i = 0; i < base8ArrayLen; i++){
        std::cout << (int)base8Array[i] << " ";
    }
    std::cout << std::endl;

    if (getChecksum(base8Array, base8ArrayLen) == 0) {return true;}
    return false;
}

// TODO: Remove once RTC is implemented.
uint32_t getTimestampSinceBoot(const uint64_t bootTimestamp){
    return (uint32_t)((time_us_64() - bootTimestamp) / 1000000);
}

// TODO: modify once RTC is implemented.
void printValidLogs(LogType logType){
    uint16_t logAddr = 0;
    uint8_t logData[LOG_ARR_LEN];
    int tmp_log_array_length = LOG_ARR_LEN;

    if (logType == LOGTYPE_MSG_LOG){
        logAddr = LOG_START_ADDR;
        for (int i = 0; i < MAX_LOGS; i++){
        eeprom_read_page(logAddr, logData, LOG_ARR_LEN);
        if (logData[LOG_USE_STATUS] == 1 && verifyDataIntegrity(logData, tmp_log_array_length) == true){
            uint8_t messageCode = logData[MESSAGE_CODE];
            uint32_t timestamp = (logData[TIMESTAMP_MSB] << 24) | (logData[TIMESTAMP_MSB1] << 16) | (logData[TIMESTAMP_MSB2] << 8) | logData[TIMESTAMP_LSB];

            std::cout << logAddr << ": " << logMessages[messageCode] << " " << timestamp << " seconds after last boot." << std::endl;
        }
        logAddr += LOG_SIZE;
        }   
    }
    else if (logType == LOGTYPE_REBOOT_STATUS){
        logAddr = REBOOT_STATUS_START_ADDR;
        for (int i = 0; i < MAX_LOGS; i++){
        eeprom_read_page(logAddr, logData, LOG_ARR_LEN);
        if (logData[LOG_USE_STATUS] == 1 && verifyDataIntegrity(logData, tmp_log_array_length) == true){
            uint8_t messageCode = logData[MESSAGE_CODE];
            uint32_t timestamp = (logData[TIMESTAMP_MSB] << 24) | (logData[TIMESTAMP_MSB1] << 16) | (logData[TIMESTAMP_MSB2] << 8) | logData[TIMESTAMP_LSB];

            std::cout << logAddr << ": " << rebootStatusMessages[messageCode] << " " << timestamp << " seconds after last boot." << std::endl;
        }
        logAddr += LOG_SIZE;
        }   
    }
    return;
}

/*void LogHandler::setCommHandler(std::shared_ptr<CommHandler> commHandler){
    this->commHandler = commHandler;
    return;
}*/

void LogHandler::receive() {
    for (;;) {
        Reading reading;
        if (xQueueReceive(receiver, &reading, pdMS_TO_TICKS(500))) {
        trap(reading);
        if (reading.type == ReadingType::FAN_COUNTER) {
            std::cout << ": " << reading.value.u16 << std::endl;
        } else if (reading.type == ReadingType::PRESSURE) {
            std::cout << ": " << reading.value.i16 << std::endl;
        } else {
            std::cout << ": " << reading.value.f32 << std::endl;
        }
        }
    }
}

QueueHandle_t LogHandler::get_queue_handle() const { return receiver; }

int createCredentialArray(std::string str, uint8_t *arr){
    int length = str.length();
    if (length > 60){
        //DPRINT("String too long.");
        // TODO: do something else to indicate that the string is too long.
        return -1;
    }
    arr[0] =  LOG_IN_USE;
    arr[1] = length + 5; // Length of the string + 2 bytes for the CRC + 2 bytes for the length.
    memcpy(arr + 2, str.c_str(), length + 1);
    return length + 3;
}

void LogHandler::storeData( float* CO2,  float* temperature,  float* rel_humidity,  int pressure){
    char co2Str[20], tempStr[20], rhStr[20], pressureStr[20];
    snprintf(co2Str, sizeof(co2Str), "%f", CO2);
    snprintf(tempStr, sizeof(tempStr), "%f", temperature);
    snprintf(rhStr, sizeof(rhStr), "%f", rel_humidity);
    snprintf(pressureStr, sizeof(pressureStr), "%f", pressure);

    uint8_t co2Arr[DATA_ARR_SIZE];
    uint8_t tempArr[DATA_ARR_SIZE];
    uint8_t rel_humARR[DATA_ARR_SIZE];
    uint8_t pressureArr[DATA_ARR_SIZE];

    int co2Len = createCredentialArray(co2Str, co2Arr);
    int tempLen = createCredentialArray(tempStr, tempArr);
    int rhLen = createCredentialArray(rhStr, rel_humARR);
    int pressureLen = createCredentialArray(pressureStr, pressureArr);

    appendCrcToBase8Array(co2Arr, &co2Len);
    appendCrcToBase8Array(tempArr, &tempLen);
    appendCrcToBase8Array(rel_humARR, &rhLen);
    appendCrcToBase8Array(pressureArr, &pressureLen);

    eeprom_write_page(this->unusedCommConfigAddr + (DATA_ARR_SIZE * CARBONDIOXIDE), co2Arr,co2Len);
    eeprom_write_page(this->unusedCommConfigAddr + (DATA_ARR_SIZE * TEMPERATURE), tempArr,tempLen);
    eeprom_write_page(this->unusedCommConfigAddr + (DATA_ARR_SIZE * REL_HUMIDITY),rel_humARR, rhLen);
    eeprom_write_page(this->unusedCommConfigAddr + (DATA_ARR_SIZE * PRESSURE), pressureArr,pressureLen);

    LogHandler::incrementUnusedLogIndex(LOGTYPE_COMM_CONFIG);

    return;
}


void LogHandler::fetchData(float *CO2, float *temperature, float *rel_humidity, int16_t *pressure, int *arr){
    uint8_t co2Arr[DATA_ARR_SIZE];
    uint8_t tempArr[DATA_ARR_SIZE];
    uint8_t rhArr[DATA_ARR_SIZE];
    uint8_t pressureArr[DATA_ARR_SIZE];

    for (int i = 0; i < 4; ++i) {
        arr[i] = 0;
    }

    eeprom_read_page(this->currentCommConfigAddr + (DATA_ARR_SIZE * 0), co2Arr, DATA_ARR_SIZE);
    eeprom_read_page(this->currentCommConfigAddr + (DATA_ARR_SIZE * 1), tempArr, DATA_ARR_SIZE);
    eeprom_read_page(this->currentCommConfigAddr + (DATA_ARR_SIZE * 2), rhArr, DATA_ARR_SIZE);
    eeprom_read_page(this->currentCommConfigAddr + (DATA_ARR_SIZE * 3), pressureArr, DATA_ARR_SIZE);

    char buffer[20]; // Temporary buffer to store string representation of float

    // Check and convert co2Arr to float CO2
    if (verifyDataIntegrity(co2Arr, (int)co2Arr[1])) {
        memcpy(buffer, co2Arr + 2, (int)co2Arr[1] - 3);
        buffer[(int)co2Arr[1] - 3] = '\0'; // Null-terminate string
        *CO2 = std::strtof(buffer, nullptr);
        arr[0] = 1;
    }

    // Check and convert tempArr to float temperature
    if (verifyDataIntegrity(tempArr, (int)tempArr[1])) {
        memcpy(buffer, tempArr + 2, (int)tempArr[1] - 3);
        buffer[(int)tempArr[1] - 3] = '\0'; // Null-terminate string
        *temperature = std::strtof(buffer, nullptr);
        arr[1] = 1;
    }

    // Check and convert rhArr to float rel_humidity
    if (verifyDataIntegrity(rhArr, (int)rhArr[1])) {
        memcpy(buffer, rhArr + 2, (int)rhArr[1] - 3);
        buffer[(int)rhArr[1] - 3] = '\0'; // Null-terminate string
        *rel_humidity = std::strtof(buffer, nullptr);
        arr[2] = 1;
    }

    // Check and convert pressureArr to float pressure
    if (verifyDataIntegrity(pressureArr, (int)pressureArr[1])) {
        memcpy(buffer, pressureArr + 2, (int)pressureArr[1] - 3);
        buffer[(int)pressureArr[1] - 3] = '\0'; // Null-terminate string
        *pressure = std::strtof(buffer, nullptr);
        arr[3] = 1;
    }

    return;
}
