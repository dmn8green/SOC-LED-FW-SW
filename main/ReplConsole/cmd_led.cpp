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
    e_set_length_op,
    e_info_op,
    e_unknown_op
} operation_t;

static struct {
    struct arg_str *command = nullptr;
    struct arg_int *pattern = nullptr;
    struct arg_int *strip_idx = nullptr;
    struct arg_int *led_length = nullptr;
    struct arg_int *charge = nullptr;
    struct arg_end *end;
} led_args;

//*****************************************************************************
static bool is_cmd_valid(const char *cmd)
{
    if (STR_IS_EQUAL(cmd, "on") || 
        STR_IS_EQUAL(cmd, "off") ||
        STR_IS_EQUAL(cmd, "pattern") ||
        STR_IS_EQUAL(cmd, "set-length") ||
        STR_IS_EQUAL(cmd, "info")) {
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
    if (STR_IS_EQUAL(command, "set-length")) { return e_set_length_op; }
    if (STR_IS_EQUAL(command, "info")) { return e_info_op; }

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

static int on_pattern(int stripIndex, int pattern, int charge)
{
    ESP_LOGI(TAG, "Setting pattern %d", pattern);

    MN8App& app = MN8App::instance();
    if (stripIndex == 0) {
        app.get_led_task_0().set_pattern((led_state_t) pattern, charge);
        app.get_led_task_1().set_pattern((led_state_t) pattern, charge);
    } else if (stripIndex == 1) {
        app.get_led_task_0().set_pattern((led_state_t) pattern, charge);
    } else if (stripIndex == 2) {
        app.get_led_task_1().set_pattern((led_state_t) pattern, charge);
    } else {
        ESP_LOGE(TAG, "Invalid strip index %d", stripIndex);
        return 1;
    }

    return 0;
}

static int on_info(void)
{
    MN8App& app = MN8App::instance();
    auto& site_config = app.get_context().get_site_config();
    // auto& thing_config = app.get_context().get_thing_config();

    printf("LED length: %d\n", site_config.get_led_length());

    return 0;
}

static int on_set_length(int length)
{
    ESP_LOGI(TAG, "Setting length %d", length);

    if (length != 60 && length != 120) {
        ESP_LOGE(TAG, "Invalid length %d, must be 60 or 100", length);
        return 1;
    }

    MN8App& app = MN8App::instance();
    auto& site_config = app.get_context().get_site_config();
    site_config.set_led_length(length);
    site_config.save();

    printf("Please reboot the device for the change to take effect\n");

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
    int charge = led_args.charge->ival[0];
    int led_length = led_args.led_length->ival[0];
    ESP_LOGI(TAG, "Strip index: %d, pattern: %d, charge: %d, led_length: %d", stripIndex, pattern, charge, led_length);

    if (led_args.strip_idx->count == 0) {
        ESP_LOGI(TAG, "No strip index given, controlling both strips");
        stripIndex = 0;
    }

    if (led_args.pattern->count == 0) {
        ESP_LOGI(TAG, "No pattern given, using pattern 0");
        pattern = 0;
    }

    if (led_args.charge->count == 0) {
        ESP_LOGI(TAG, "No charge given, using 0");
        charge = 0;
    }

    operation_t op = infer_operation_from_args();
    switch (op) {
        case e_on_op:
            return on_on_op(stripIndex);
        case e_off_op:
            return on_off(stripIndex);
        case e_pattern_op:
            return on_pattern(stripIndex, pattern, charge);
        case e_set_length_op:
            return on_set_length(led_length);
        case e_info_op:
            return on_info();
        default:
            return 1;
    }


    return 0;
}

void register_led(void)
{
    led_args.command = arg_str1(NULL, NULL, "<on/off/pattern/sed-length/info>", "Command to execute");
    led_args.pattern = arg_int0("p", "pattern", "<p>", "Index to the desired pattern, 0 by default");
    led_args.strip_idx = arg_int0("s", "strip", "<1/2>", "Which strip to control. If no value is given, both strips will be controlled");
    led_args.charge = arg_int0("c", "charge", "<0-100>", "Charge percentage. 0 by default");
    led_args.led_length = arg_int0("l", "length", "<60/100>", "Set the length of the LED strip. 60 or 100. 100 by default");
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
