#pragma once

#include <stdint.h>
#include "esp_console.h"

void repl_configure(uint16_t txPin, uint16_t rxPin, uint16_t channel, uint32_t baudRate);
void repl_start(void);
