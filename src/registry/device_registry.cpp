#include "device_registry.h"
#include "fan_controller.h"
#include "register_handler.h"

DeviceRegistry::DeviceRegistry() {
    xTaskCreate(initialize_task, "initialize_registry", 512, this, tskIDLE_PRIORITY + 4, NULL);
}

// Add shared pointers to the registry
void DeviceRegistry::add_shared(shared_modbus sh_mb, shared_i2c sh_i2c) {
    mbctrl = sh_mb;
    i2c = sh_i2c;
}

// Setup task to create register handlers and add them to the registry
void DeviceRegistry::initialize() {
    auto co2 = std::make_shared<ModbusReadHandler>(mbctrl, 240, 0x0, 2, true, ReadingType::CO2,
                                                   "CO2 Level");
    auto temp = std::make_shared<ModbusReadHandler>(mbctrl, 241, 0x2, 2, true,
                                                    ReadingType::TEMPERATURE, "Temp");
    auto hum = std::make_shared<ModbusReadHandler>(mbctrl, 241, 0x0, 2, true,
                                                   ReadingType::REL_HUMIDITY, "Hum");
    auto fan_counter = std::make_shared<ModbusReadHandler>(mbctrl, 1, 4, 1, false,
                                                           ReadingType::FAN_COUNTER, "Fan Counter");
    auto w_fan_speed = std::make_shared<ModbusWriteHandler>(mbctrl, 1, 0, 1, WriteType::FAN_SPEED,
                                                          "Fan Speed Control");
    auto pressure = std::make_shared<I2CHandler>(i2c, 0x40, ReadingType::PRESSURE, "Pressure");

    fanctrl = std::make_shared<FanController>(w_fan_speed->get_write_queue_handle(), 400.0);
    add_register_handler(std::move(co2), ReadingType::CO2);
    add_register_handler(std::move(temp), ReadingType::TEMPERATURE);
    add_register_handler(std::move(hum), ReadingType::REL_HUMIDITY);
    add_register_handler(std::move(fan_counter), ReadingType::FAN_COUNTER);
    add_register_handler(std::move(w_fan_speed), WriteType::FAN_SPEED);
    add_register_handler(std::move(pressure), ReadingType::PRESSURE);


    subscribe_to_handler(ReadingType::CO2, fanctrl->get_reading_queue_handle());

    auto r_fan_speed = std::make_shared<FanSpeedReadHandler>(fanctrl);
    auto co2_target = std::make_shared<CO2TargetReadHandler>(fanctrl);

    vTaskSuspend(nullptr);
}

// Subscribe to a specific read handler if it exists
// Subscriber must specify ReadingType and give a queue handle
void DeviceRegistry::subscribe_to_handler(const ReadingType type, QueueHandle_t receiver) {
    if (read_handlers.find(type) != read_handlers.end()) {
        read_handlers[type]->add_subscriber(receiver);
        std::cout << "Subscriber added to " << read_handlers[type]->get_name() << std::endl;
    } else {
        std::cout << "Handler not found" << std::endl;
    }
}

// TODO: this should probably first redirect to FanController and then to ModbusHandler
//
// Get write queue handle for a specific write handler
// Subscriber must specify WriteType and will get a queue handle in return if it exists
QueueHandle_t DeviceRegistry::get_write_queue_handle(const WriteType type) {
    if (write_handlers.find(type) != write_handlers.end()) {
        std::cout << "Subscriber given handle to " << write_handlers[type]->get_name() << std::endl;
        if (type == WriteType::FAN_SPEED || type == WriteType::CO2_TARGET) { // TODO: these are technically right now the only write handlers
            return fanctrl->get_write_queue_handle();
        } else { // TODO: so this is not really necessary at all rn
            return write_handlers[type]->get_write_queue_handle();
        }
    }
    std::cout << "Handler not found" << std::endl;
    return nullptr;
}

// Subscribe to all read handlers in the registry
// Subscriber must give a queue handle
void DeviceRegistry::subscribe_to_all(QueueHandle_t receiver) {
    std::cout << "All subscribers added" << std::endl;
    for (auto &[rtype, handler] : read_handlers) {
        handler->add_subscriber(receiver);
    }
}

// Add a read handler to the registry unless one of the same type already exists
void DeviceRegistry::add_register_handler(std::shared_ptr<ReadRegisterHandler> handler,
                                          const ReadingType type) {
    for (const auto &[rtype, rhandler] : read_handlers) {
        if (type == rtype) {
            std::cout << "Handler already exists" << std::endl;
            return;
        }
    }
    read_handlers[type] = std::move(handler);
    std::cout << "Read handler added" << std::endl;
}

// Add a write handler to the registry unless one of the same type already exists
void DeviceRegistry::add_register_handler(std::shared_ptr<WriteRegisterHandler> handler,
                                          const WriteType type) {
    for (const auto &[wtype, whandler] : write_handlers) {
        if (type == wtype) {
            std::cout << "Handler already exists" << std::endl;
            return;
        }
    }
    write_handlers[type] = std::move(handler);
    std::cout << "Write handler added" << std::endl;
}

//
// Test stuff only under this comment
//
TestSubscriber::TestSubscriber() {
    receiver = xQueueCreate(10, sizeof(Reading));
    xTaskCreate(receive_task, "Receive Task", 256, this, tskIDLE_PRIORITY + 2, nullptr);
}

TestSubscriber::TestSubscriber(const std::string &name) : name(name) {
    receiver = xQueueCreate(10, sizeof(Reading));
    xTaskCreate(receive_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 2, nullptr);
}

void TestSubscriber::receive() {
    for (;;) {
        Reading reading;
        if (xQueueReceive(receiver, &reading, pdMS_TO_TICKS(500))) {
            trap(reading);
            if (reading.type == ReadingType::FAN_COUNTER) {
                std::cout << name << ": " << reading.value.u16 << std::endl;
            } else if (reading.type == ReadingType::PRESSURE) {
                std::cout << name << ": " << reading.value.i16 << std::endl;
            } else {
                std::cout << name << ": " << reading.value.f32 << std::endl;
            }
        }
    }
}

QueueHandle_t TestSubscriber::get_queue_handle() const { return receiver; }

TestWriter::TestWriter() {
    this->name = "TestWriter";
    this->send_handle = nullptr;
}

TestWriter::TestWriter(const std::string &name, QueueHandle_t handle) {
    this->name = name;
    this->send_handle = handle;

    xTaskCreate(send_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 2, nullptr);
}

void TestWriter::add_send_handle(QueueHandle_t handle) { send_handle = handle; }

void TestWriter::send() const {
    uint32_t msg = 400;
    for (;;) {
        xQueueSend(send_handle, &msg, 0);
        msg = 0;
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void trap(Reading reading) {
    // std::cout << reading.value.u16 << std::endl;
}
