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
    Network/ethernet_init.cpp
    Network/NetworkInterface.cpp
    Network/NetworkConfiguration.cpp
    Network/WifiConfiguration.cpp
    Network/WifiConnection.cpp
    Network/EthConfiguration.cpp
    Network/EthConnection.cpp
    Network/NetworkManager.cpp
    MN8App.cpp
    main.cpp
)

set(COMPONENT_ADD_INCLUDEDIRS
    "."
)

register_component()