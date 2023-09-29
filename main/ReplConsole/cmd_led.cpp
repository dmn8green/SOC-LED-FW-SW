#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_console.h"
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "esp_flash.h"

#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "App/MN8App.h"

#define STR_IS_EQUAL(str1, str2) (strncasecmp(str1, str2, strlen(str2)) == 0)
#define STR_NOT_EMPTY(str) (strlen(str) > 0)
#define STR_IS_EMPTY(str) (strlen(str) == 0)



static const char* TAG = "cmd_led";

typedef enum {
    e_on_op,
    e_off_op,
    e_pattern_op,
    e_unknown_op
} operation_t;

static struct {
    struct arg_str *command = nullptr;
    struct arg_int *pattern = nullptr;
    struct arg_int *strip_idx = nullptr;
    struct arg_end *end;
} led_args;

//*****************************************************************************
static bool is_cmd_valid(const char *cmd)
{
    if (STR_IS_EQUAL(cmd, "on") || 
        STR_IS_EQUAL(cmd, "off") ||
        STR_IS_EQUAL(cmd, "pattern")) {
        return true;
    }
    return false;
}

//*****************************************************************************
static operation_t infer_operation_from_args(void) {
    const char* command = led_args.command->sval[0];

    if (!is_cmd_valid(command)) {
        printf("ERROR: %s is not a valid command", command);
        return e_unknown_op;
    }

    if (STR_IS_EQUAL(command, "off")) { return e_off_op; }
    if (STR_IS_EQUAL(command, "on")) { return e_on_op; }
    if (STR_IS_EQUAL(command, "pattern")) { return e_pattern_op; }

    return e_unknown_op;
}

static int on_on_op(int stripIndex)
{
    ESP_LOGI(TAG, "Turning on strip %d", stripIndex);

    MN8App& app = MN8App::instance();
    if (stripIndex == 0) {
        //app.get_led_task_0().resume();
        app.get_led_task_1().resume();
    } else if (stripIndex == 1) {
        //app.get_led_task_0().resume();
    } else if (stripIndex == 2) {
        app.get_led_task_1().resume();
    } else {
        ESP_LOGE(TAG, "Invalid strip index %d", stripIndex);
        return 1;
    }
    return 0;
}

static int on_off(int stripIndex)
{
    ESP_LOGI(TAG, "Turning off strip %d", stripIndex);

    MN8App& app = MN8App::instance();
    if (stripIndex == 0) {
        //app.get_led_task_0().suspend();
        app.get_led_task_1().suspend();
    } else if (stripIndex == 1) {
        //app.get_led_task_0().suspend();
    } else if (stripIndex == 2) {
        app.get_led_task_1().suspend();
    } else {
        ESP_LOGE(TAG, "Invalid strip index %d", stripIndex);
        return 1;
    }

    return 0;
}

static int on_pattern(int stripIndex, int pattern)
{
    ESP_LOGI(TAG, "Setting pattern %d", pattern);

    MN8App& app = MN8App::instance();
    if (stripIndex == 0) {
        app.get_led_task_0().set_pattern((led_state_t) pattern, 0);
        app.get_led_task_1().set_pattern((led_state_t) pattern, 0);
    } else if (stripIndex == 1) {
        app.get_led_task_0().set_pattern((led_state_t) pattern, 1);
    } else if (stripIndex == 2) {
        app.get_led_task_1().set_pattern((led_state_t) pattern, 0);
    } else {
        ESP_LOGE(TAG, "Invalid strip index %d", stripIndex);
        return 1;
    }

    return 0;
}

static int led_task(int argc, char **argv)
{
    ESP_LOGI(TAG, "Number of args: %d", argc);


    int nerrors = arg_parse(argc, argv, (void **) &led_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, led_args.end, argv[0]);
        return 1;
    }

    int stripIndex = led_args.strip_idx->ival[0];
    int pattern = led_args.pattern->ival[0];

    if (led_args.strip_idx->count == 0) {
        ESP_LOGI(TAG, "No strip index given, controlling both strips");
        stripIndex = 0;
    }

    if (led_args.pattern->count == 0) {
        ESP_LOGI(TAG, "No pattern given, using pattern 0");
        pattern = 0;
    }

    operation_t op = infer_operation_from_args();
    switch (op) {
        case e_on_op:
            return on_on_op(stripIndex);
        case e_off_op:
            return on_off(stripIndex);
        case e_pattern_op:
            return on_pattern(stripIndex, pattern);
        default:
            return 1;
    }


    return 0;
}

void register_led(void)
{
    led_args.command = arg_str1(NULL, NULL, "<on/off/pattern>", "Command to execute");
    led_args.pattern = arg_int0("p", "pattern", "<p>", "Index to the desired pattern, 0 by default");
    led_args.strip_idx = arg_int0("s", "strip", "<1/2>", "Which strip to control. If no value is given, both strips will be controlled");
    led_args.end = arg_end(1);

    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    const esp_console_cmd_t cmd = {
        .command = "led",
        .help = "Control LED strips",
        .hint = NULL,
        .func = &led_task,
        .argtable = &led_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
