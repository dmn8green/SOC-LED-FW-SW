#pragma once

#include "Utils/FreeRTOSTask.h"
#include "App/MqttAgent/MqttAgent.h"
#include "App/Configuration/ThingConfig.h"

#define IOT_HEARTBEAT_TASK_STACK_SIZE 4096
#define IOT_HEARTBEAT_TASK_PRIORITY 7
#define IOT_HEARTBEAT_TASK_CORE_NUM 0
#define IOT_HEARTBEAT_TASK_NAME "iot_heartbeat"

class IotHeartbeat : public FreeRTOSTask {
public:
    IotHeartbeat(void) : FreeRTOSTask(
        IOT_HEARTBEAT_TASK_STACK_SIZE,
        IOT_HEARTBEAT_TASK_PRIORITY,
        IOT_HEARTBEAT_TASK_CORE_NUM
    ) {};
    ~IotHeartbeat(void) = default;

public:
    esp_err_t setup(MqttAgent* mqtt_context, ThingConfig* thing_config);
    virtual const char* task_name(void) override { return IOT_HEARTBEAT_TASK_NAME; }

protected:
    virtual void taskFunction(void) override;
    
private:
    MqttAgent* mqtt_context;
    ThingConfig* thing_config;

};