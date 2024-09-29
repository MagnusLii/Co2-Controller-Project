#include "DeviceRegistry.h"
#include "FreeRTOS.h"
#include "PicoOsUart.h"
#include "RegisterHandler.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "semphr.h"
#include "ssd1306.h"
#include "task.h"
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include "modbus_controller.h"
#include "modbus_register.h"
#include "uart_instance.h"

#include <limits.h>

extern "C" {
uint32_t read_runtime_ctr(void) { return timer_hw->timerawl; }
}

#define UART_TX_PIN 4
#define UART_RX_PIN 5

void init_modbus_handlers(DeviceRegistry &manager, shared_modbus mbctrl) {}
/*
struct ControlMessage{
    WriteType type;
    uint32_t value;
};
*/
class DeviceManager : public DeviceRegistry {
  public:
    DeviceManager();
    void subscribe_to_handler(ReadingType type, QueueHandle_t receiver);
    QueueHandle_t get_relay_queue_handle();
    void print_debug_info();

  private:
    void read_sensors();
    void send_readings();
    void relay_ctrl_messages();

    static void read_sensors_task(void *pvParameters) {
        auto dm = static_cast<DeviceManager *>(pvParameters);
        dm->read_sensors();
    }

    static void send_readings_task(void *pvParameters) {
        auto dm = static_cast<DeviceManager *>(pvParameters);
        dm->send_readings();
    }

    static void send_control_messages_task(void *pvParameters) {
        auto dm = static_cast<DeviceManager *>(pvParameters);
        dm->relay_ctrl_messages();
    }

    std::map<ReadingType, Reading> readings;
    std::map<ReadingType, std::vector<QueueHandle_t>> subscribers;
    QueueHandle_t relay_queue;
    bool readings_ready = false;
};

union SensorValue {
    uint32_t u32;
    float f32;
    int32_t i32;
    uint16_t u16;
};

class SensorReader {
  public:
    SensorReader();
    void add_sensor(ReadingType type, std::shared_ptr<ReadRegisterHandler> handler);

  private:
    void read_sensors();
    static void read_sensors_task(void *pvParameters) {
        auto sr = static_cast<SensorReader *>(pvParameters);
        sr->read_sensors();
    }

    std::map<ReadingType, std::shared_ptr<ReadRegisterHandler>> handlers;
    std::map<ReadingType, Reading> readings;
    std::vector<TimerHandle_t> read_timers;
};

int main() {
    stdio_init_all();
    std::cout << "Booting" << std::endl;

    shared_uart uart_i{std::make_shared<Uart_instance>(1, 9600, UART_TX_PIN, UART_RX_PIN,
                                                       2)}; // 1 for testbox 2 for fullscale
    shared_modbus mbctrl{std::make_shared<ModbusCtrl>(uart_i)};

    //DeviceManager manager;
    SensorReader reader;

    auto co2_handler = std::make_shared<ModbusReadHandler>(mbctrl, 240, 0x0, 2, true, ReadingType::CO2, "co2");
    auto temp_handler = std::make_shared<ModbusReadHandler>(mbctrl, 241, 0x2, 2, true, ReadingType::TEMPERATURE, "temp");
    auto rh_handler = std::make_shared<ModbusReadHandler>(mbctrl, 241, 0x0, 2, true, ReadingType::REL_HUMIDITY, "rh");
    auto fan_counter_handler = std::make_shared<ModbusReadHandler>(mbctrl, 1, 0x4, 1, true, ReadingType::FAN_COUNTER, "fcounter");
    auto fan_speed_handler = std::make_shared<ModbusWriteHandler>(mbctrl, 1, 0x6, 1, WriteType::FAN_SPEED, "fan_speed");

    reader.add_sensor(ReadingType::CO2, co2_handler);
    reader.add_sensor(ReadingType::TEMPERATURE, temp_handler);
    reader.add_sensor(ReadingType::REL_HUMIDITY, rh_handler);
    reader.add_sensor(ReadingType::FAN_COUNTER, fan_counter_handler);

    //manager.add_register_handler(ReadingType::CO2, co2_handler);
    //manager.add_register_handler(ReadingType::TEMPERATURE, temp_handler);
    //manager.add_register_handler(ReadingType::REL_HUMIDITY, rh_handler);
    //manager.add_register_handler(ReadingType::FAN_COUNTER, fan_counter_handler);
    //manager.add_register_handler(WriteType::FAN_SPEED, fan_speed_handler);

    //TestSubscriber eeprom("CO2");
    //TestSubscriber display("TEMPERATURE");
    //TestSubscriber wifi("REL_HUMIDITY");
    //TestSubscriber fan_fan("FAN_COUNTER");
    //TestWriter writer("Controller", manager.get_relay_queue_handle());

    //manager.subscribe_to_handler(ReadingType::CO2, eeprom.get_queue_handle());
    //manager.subscribe_to_handler(ReadingType::TEMPERATURE, display.get_queue_handle());
    //manager.subscribe_to_handler(ReadingType::REL_HUMIDITY, wifi.get_queue_handle());
    //manager.subscribe_to_handler(ReadingType::FAN_COUNTER, fan_fan.get_queue_handle());

    //manager.print_debug_info();

    vTaskStartScheduler();

    for (;;) {
    }
}

SensorReader::SensorReader() {
    xTaskCreate(read_sensors_task, "read_sensors_task", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void SensorReader::add_sensor(ReadingType type, std::shared_ptr<ReadRegisterHandler> handler) {
    handlers[type] = handler;
    read_timers.push_back(handler->get_timer_handle());
}

void SensorReader::read_sensors() {
    // Start timers
    //for (auto timer : read_timers) {
    //    xTimerStart(timer, 0);
    //}
    vTaskDelay(pdMS_TO_TICKS(1000));


    for (;;) {
        for (auto const& [type, handler] : handlers) {
            Reading new_reading = handler->get_reading();
            std::cout << "Reading : " << new_reading.value.f32 << std::endl;

        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


DeviceManager::DeviceManager() : DeviceRegistry() {
    relay_queue = xQueueCreate(5, sizeof(QueueHandle_t));

    xTaskCreate(send_readings_task, "send_readings_task", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(send_control_messages_task, "send_control_messages_task", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(read_sensors_task, "read_sensors_task", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}


void DeviceManager::subscribe_to_handler(ReadingType type, QueueHandle_t receiver) {
    for (auto const& handler_pair : read_handlers) {
        if (handler_pair.first == type) {
            subscribers[type].push_back(receiver);
        }
    }
}


void DeviceManager::read_sensors() {
    print_debug_info();
    vTaskDelay(pdMS_TO_TICKS(1000));
    for (;;) {
//        for (auto const& [type, handler] : read_handlers) {
//            Reading new_read;
//            new_read.type = type;
//            new_read.value.u32 = handler->get_reading();
//            readings[type] = new_read;
//        }
//
        readings_ready = true;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void DeviceManager::send_readings() {
    print_debug_info();
    vTaskDelay(pdMS_TO_TICKS(1000));
    for (;;) {
        if (readings_ready) {
            std::cout << "Sending readings" << std::endl;
            //for (auto reading : readings) {
            //    for (auto subscriber : subscribers[reading.first]) {
            //        xQueueSend(subscriber, &reading.second, 0);
            //    }
            //}
            readings_ready = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void DeviceManager::relay_ctrl_messages() {
    print_debug_info();
    ControlMessage msg{};
    if (readings_ready) {
        std::cout << "ready readings" << std::endl;
    } else {
        std::cout << "no readings" << std::endl;
    }

    for (;;) {
//        if (xQueueReceive(relay_queue, &msg, portMAX_DELAY)) {
//            if (write_handlers.find(msg.type) != write_handlers.end()) {
//                write_handlers[msg.type]->write_to_reg(msg.value);
//            }
//        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

QueueHandle_t DeviceManager::get_relay_queue_handle() {
    return relay_queue;
}

void DeviceManager::print_debug_info() {
    std::cout << "DEBUG INFO" << std::endl;
    std::cout << "Read handlers" << std::endl;
    for (auto const& handler_pair : read_handlers) {
        std::cout << ": " << handler_pair.second->get_name() << std::endl;
    }

    std::cout << "Write handlers" << std::endl;
    for (auto const& handler_pair : write_handlers) {
        std::cout << ": " << handler_pair.second->get_name() << std::endl;
    }

    std::cout << "Subscribers" << std::endl;
    for (auto const& sub : subscribers) {
        std::cout << ": Sub" << std::endl;
    }

}
