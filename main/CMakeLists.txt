# idf_component_register( SRCS "led_strip_example_main.c" "led_strip_encoder.c"
#                         INCLUDE_DIRS ".")

set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )
set(COMPONENT_SRCS
    ReplConsole/repl_console.cpp
    ReplConsole/cmd_tasks.cpp
    ReplConsole/cmd_wifi.cpp
    main.cpp
)

set(COMPONENT_ADD_INCLUDEDIRS
    "."
)

register_component()