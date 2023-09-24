# idf_component_register( SRCS "led_strip_example_main.c" "led_strip_encoder.c"
#                         INCLUDE_DIRS ".")


## Because we are including local components, we need to also specifically include
## ESP components that we are using.

set(COMPONENT_REQUIRES
    console
    fatfs
    esp_wifi
    esp_http_client
    esp_eth
    coreMQTT
    coreMQTT-Agent
    corePKCS11
    coreJSON
    backoffAlgorithm
    esp_secure_cert_mgr
    ota-for-aws-iot-embedded-sdk
    unity
    driver
    ArduinoJson
    soc
)

#set(COMPONENT_PRIV_REQUIRES )
set(COMPONENT_SRCS
    Utils/KeyStore.cpp
    Utils/FuseMacAddress.cpp
    Utils/HSV2RGB.cpp
    IOT/ThingConfig.cpp
    LED/LedTaskSpi.cpp
    LED/RmtOverSpi.cpp
    LED/Animations/ChasingAnimation.cpp
    LED/Animations/PulsingAnimation.cpp
    LED/Animations/StaticAnimation.cpp
    LED/Animations/SmoothRatePulseCurve.cpp
    LED/Animations/ChargingAnimation.cpp
    ReplConsole/repl_console.cpp
    ReplConsole/cmd_tasks.cpp
    ReplConsole/cmd_wifi.cpp
    ReplConsole/cmd_ifconfig.cpp
    ReplConsole/cmd_reboot.cpp
    ReplConsole/cmd_ping.cpp
    ReplConsole/cmd_provision_iot.cpp
    ReplConsole/cmd_led.cpp
    Network/NetworkInterface.cpp
    Network/Utils/ethernet_init.cpp
    Network/Connection/Connection.cpp
    Network/Connection/WifiConnection.cpp
    Network/Connection/EthConnection.cpp
    Network/Configuration/NetworkConfiguration.cpp
    Network/Configuration/WifiConfiguration.cpp
    Network/Configuration/EthConfiguration.cpp
    Network/MQTT/MqttAgent.cpp
#    Network/MQTT/MQTTContext.cpp
#    Network/MQTT/subscription_manager.c
#    Network/MQTT/core_mqtt_agent_manager.c
#    Network/MQTT/core_mqtt_agent_manager_events.c
    Network/MQTT/mqtt_demo_mutual_auth.c
    MN8App.cpp
    main.cpp
)

set(COMPONENT_ADD_INCLUDEDIRS
    "."
)


register_component()