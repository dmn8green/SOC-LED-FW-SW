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
    esp_https_server
)

#set(COMPONENT_PRIV_REQUIRES )
set(COMPONENT_SRCS
    Utils/KeyStore.cpp
    Utils/FuseMacAddress.cpp
    Utils/HSV2RGB.cpp
    Utils/Colors.cpp
    Utils/FreeRTOSTask.cpp
    Utils/iot_provisioning.cpp
    Utils/Updater.cpp
    LED/LedTaskSpi.cpp
    LED/RmtOverSpi.cpp
    #LED/Animations/ChasingAnimation.cpp
    LED/Animations/ChargeIndicator.cpp
    LED/Animations/ProgressAnimation.cpp
    LED/Animations/PulsingAnimation.cpp
    LED/Animations/StaticAnimation.cpp
    LED/Animations/SmoothRatePulseCurve.cpp
    LED/Animations/ChargingAnimation.cpp
    LED/Animations/ChargingAnimationWhiteBubble.cpp
    ReplConsole/repl_console.cpp
    ReplConsole/cmd_factory_reset.cpp
    ReplConsole/cmd_tasks.cpp
    ReplConsole/cmd_wifi.cpp
    ReplConsole/cmd_ifconfig.cpp
    ReplConsole/cmd_reboot.cpp
    ReplConsole/cmd_ping.cpp
    ReplConsole/cmd_iot.cpp
    ReplConsole/cmd_chargepoint.cpp
    ReplConsole/cmd_led.cpp
    ReplConsole/cmd_info.cpp
    Network/NetworkInterface.cpp
    Network/Utils/ethernet_init.cpp
    Network/Connection/Connection.cpp
    Network/Connection/WifiConnection.cpp
    Network/Connection/EthConnection.cpp
    Network/Configuration/NetworkConfiguration.cpp
    Network/Configuration/WifiConfiguration.cpp
    Network/Configuration/EthConfiguration.cpp
    App/Configuration/ThingConfig.cpp
    App/Configuration/ChargePointConfig.cpp
    App/Configuration/SiteConfig.cpp
    App/NetworkAgent/NetworkConnection.cpp
    App/NetworkAgent/NetworkConnectionAgent.cpp
    App/NetworkAgent/NetworkConnectionStateMachine.cpp
    App/MqttAgent/MqttAgent.cpp
    App/MqttAgent/MqttContext.cpp
    App/MqttAgent/MqttConnection.cpp
    App/MqttAgent/IotThing.cpp
    App/IotHeartbeat.cpp
    App/MN8Context.cpp
    App/MN8StateMachine.cpp
    App/MN8App.cpp
    App/mdns_broadcaster.cpp
    App/udp_server.cpp
    App/web_server.cpp
    main.cpp
)

set(COMPONENT_ADD_INCLUDEDIRS
    "."
)

register_component()
