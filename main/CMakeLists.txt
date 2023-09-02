# idf_component_register( SRCS "led_strip_example_main.c" "led_strip_encoder.c"
#                         INCLUDE_DIRS ".")

set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )
set(COMPONENT_SRCS
    Utils/KeyStore.cpp
    ReplConsole/repl_console.cpp
    ReplConsole/cmd_tasks.cpp
    ReplConsole/cmd_wifi.cpp
    ReplConsole/cmd_ifconfig.cpp
    ReplConsole/cmd_reboot.cpp
    ReplConsole/cmd_ping.cpp
    Network/NetworkInterface.cpp
    Network/Utils/ethernet_init.cpp
    Network/Connection/Connection.cpp
    Network/Connection/WifiConnection.cpp
    Network/Connection/EthConnection.cpp
    Network/Configuration/NetworkConfiguration.cpp
    Network/Configuration/WifiConfiguration.cpp
    Network/Configuration/EthConfiguration.cpp
    MN8App.cpp
    main.cpp
)

set(COMPONENT_ADD_INCLUDEDIRS
    "."
)

register_component()