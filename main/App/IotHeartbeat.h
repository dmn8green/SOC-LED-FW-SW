#pragma once

#include "Utils/FreeRTOSTask.h"
#include "App/MqttAgent/MqttAgent.h"
#include "App/Configuration/ThingConfig.h"

class MN8StateMachine;

#define IOT_HEARTBEAT_TASK_STACK_SIZE 4096
#define IOT_HEARTBEAT_TASK_PRIORITY 7
#define IOT_HEARTBEAT_TASK_CORE_NUM 0
#define IOT_HEARTBEAT_TASK_NAME "iot_heartbeat"
#define HEARTBEAT_FREQUENCY_MINUTES  (5)

class IotHeartbeat : public FreeRTOSTask {
public:
    IotHeartbeat(void) : FreeRTOSTask(
        IOT_HEARTBEAT_TASK_STACK_SIZE,
        IOT_HEARTBEAT_TASK_PRIORITY,
        IOT_HEARTBEAT_TASK_CORE_NUM
    ) {};
    ~IotHeartbeat(void) = default;

public:
    esp_err_t setup(MqttAgent* mqtt_context, ThingConfig* thing_config, MN8StateMachine *state_machine);
    uint16_t get_heartbeat_frequency(void) { return this->heartbeat_frequency; }
    void set_heartbeat_frequency(uint16_t heartbeat_frequency) { this->heartbeat_frequency = heartbeat_frequency; }
    void send_heartbeat(void);
    virtual const char* task_name(void) override { return IOT_HEARTBEAT_TASK_NAME; }

protected:
    virtual void taskFunction(void) override;
    
private:
    MqttAgent* mqtt_context;
    ThingConfig* thing_config;
    uint16_t heartbeat_frequency = HEARTBEAT_FREQUENCY_MINUTES;
    MN8StateMachine *state_machine;
};