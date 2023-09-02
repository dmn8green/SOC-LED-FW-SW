#include "ReplConsole/repl_console.h"
#include "MN8App.h"
#include "pin_def.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

extern "C" void app_main(void)
{
    MN8App& app = MN8App::instance();
    app.setup();

    repl_configure(REPL_UART_TX_PIN, REPL_UART_RX_PIN, REPL_UART_CHANNEL, REPL_UART_BAUD_RATE);
    repl_start();

    while(1) {
        app.loop();
    }
}