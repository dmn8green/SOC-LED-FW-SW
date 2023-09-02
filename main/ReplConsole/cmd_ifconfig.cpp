#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_console.h"
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "esp_flash.h"
#include "lwip/ip_addr.h"

#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "Network/Connection/Connection.h"
#include "MN8App.h"

// ifconfig interface up/down
// ifconfig interface ip, netmask, gateway

static struct {
    struct arg_str *interface;
    struct arg_str *command;
    struct arg_str *ip;
    struct arg_str *netmask;
    struct arg_str *gateway;
    struct arg_str *dns;
    struct arg_end *end;
} ifconfig_args;

#define STR_IS_EQUAL(str1, str2) (strncasecmp(str1, str2, strlen(str2)) == 0)
#define STR_NOT_EMPTY(str) (strlen(str) > 0)
#define STR_IS_EMPTY(str) (strlen(str) == 0)

static void print_interface_information(Connection *connection)
{
    printf("\nInterface information:\n");
    connection->dump_connection_info();
    // printf("  %-20s: %s\n", "Interface", connection->get_name());
    // printf("  %-20s: %s\n", "Is Up", connection->is_enabled() ? "Yes" : "No");

    // if (connection->is_enabled()) {
    //     printf("\nCurrent connection state:\n");
    //     printf("  %-20s: %s\n", "Connected", connection->is_connected() ? "Yes" : "No");
    //     printf("  %-20s: %s\n", "DHCP", connection->is_dhcp() ? "Yes" : "No");
    // }

    // if (connection->is_connected()) {
    //     esp_ip4_addr_t ip = connection->get_ip_address();
    //     esp_ip4_addr_t netmask = connection->get_netmask();
    //     esp_ip4_addr_t gateway = connection->get_gateway();
    //     esp_ip4_addr_t dns = connection->get_dns();
        
    //     printf("  %-20s: " IPSTR "\n", "IP Address", IP2STR(&ip));
    //     printf("  %-20s: " IPSTR "\n", "Netmask", IP2STR(&netmask));
    //     printf("  %-20s: " IPSTR "\n", "Gateway", IP2STR(&gateway));
    //     printf("  %-20s: " IPSTR "\n", "DNS", IP2STR(&dns));
    // }

    printf("\nCurrent connection saved settings:\n");
    connection->dump_config();

    printf("~~~~~~~~~~~\n");
}

typedef enum {
    e_print_interface_information_op,
    e_interface_up_op,
    e_interface_down_op,
    e_interface_dhcp_op,
    e_interface_manual_op,
    e_interface_reset_conf_op,
    e_interface_dump_conf_op,
    e_unknown_op
} operation_t;

//*****************************************************************************
static bool is_eth_valid(const char* eth)
{
    if (STR_IS_EQUAL(eth, "eth") || STR_IS_EQUAL(eth, "ethernet")) {
        return true;
    }
    if (STR_IS_EQUAL(eth, "wifi") || STR_IS_EQUAL(eth, "wireless")) {
        return true;
    }
    return false;
}

//*****************************************************************************
static bool is_cmd_valid(const char *cmd)
{
    if (STR_IS_EQUAL(cmd, "up") || STR_IS_EQUAL(cmd, "down")) {
        return true;
    }
    if (STR_IS_EQUAL(cmd, "dhcp") || STR_IS_EQUAL(cmd, "manual")) {
        return true;
    }

    // secret commands
    if (STR_IS_EQUAL(cmd, "reset") || STR_IS_EQUAL(cmd, "dump_conf")) {
        return true;
    }

    return false;
}

//*****************************************************************************
static operation_t infer_operation_from_args(void) {
    const char* interface = ifconfig_args.interface->sval[0];
    const char* command = ifconfig_args.command->sval[0];
    const char* ip = ifconfig_args.ip->sval[0];
    const char* netmask = ifconfig_args.netmask->sval[0];
    const char* gateway = ifconfig_args.gateway->sval[0];
    const char* dns = ifconfig_args.dns->sval[0];

    if (STR_IS_EMPTY(interface)) {
        return e_print_interface_information_op;
    }

    // Interface was specified.  Make sure it is valid.
    if (!is_eth_valid(interface)) {
        printf("ERROR: %s is not a valid interface", interface);
        return e_unknown_op;
    }

    // No command, print interface information
    if (STR_IS_EMPTY(command)) {
        return e_print_interface_information_op;
    }

    if (!is_cmd_valid(command)) {
        printf("ERROR: %s is not a valid command", command);
        return e_unknown_op;
    }

    if (STR_IS_EQUAL(command, "up"))         { return e_interface_up_op; }
    if (STR_IS_EQUAL(command, "down"))       { return e_interface_down_op; }
    if (STR_IS_EQUAL(command, "dhcp"))       { return e_interface_dhcp_op; }
    if (STR_IS_EQUAL(command, "reset"))      { return e_interface_reset_conf_op; }
    if (STR_IS_EQUAL(command, "dump_conf"))  { return e_interface_dump_conf_op; }

    if (STR_IS_EQUAL(command, "manual")) {
        if (STR_IS_EMPTY(ip) || STR_IS_EMPTY(netmask) || STR_IS_EMPTY(gateway) || STR_IS_EMPTY(dns)) {
            printf("ERROR: IP, netmask and gateway must be specified for manual configuration");
            return e_unknown_op;
        }

        return e_interface_manual_op;
    }

    return e_unknown_op;
}

//*****************************************************************************
static int op_interface_up(Connection *connection)
{
    printf("Turning on interface %s\n", connection->get_name());
    return (connection->up(true) == ESP_OK) ? 0 : 1;
}

//*****************************************************************************
static int op_interface_down(Connection *connection)
{
    printf("Turning off interface %s\n", connection->get_name());
    return (connection->down(true) == ESP_OK) ? 0 : 1;
}

//*****************************************************************************
static int op_interface_dhcp(Connection *connection)
{
    printf("Turning on dhcp on interface %s\n", connection->get_name());
    return (connection->use_dhcp(true) == ESP_OK) ? 0 : 1;
}

//*****************************************************************************
static int op_interface_reset_conf(Connection *connection)
{
    printf("Reseting configuration for interface %s\n", connection->get_name());
    return (connection->reset_config() == ESP_OK) ? 0 : 1;
}

//*****************************************************************************
static int op_interface_dump_conf(Connection *connection)
{
    printf("Dumping configuration for interface %s\n", connection->get_name());
    return (connection->dump_config() == ESP_OK) ? 0 : 1;
}


//*****************************************************************************
static int op_interface_manual(Connection *connection)
{
#define IPSTR_TO_ESPIP(ipstr, espip, msg) \
    if (ip4addr_aton(ipstr, &espip) == 0) { \
        printf(msg " %s\n", ipstr); \
        return 1; \
    } 

    const char * ip_str      = ifconfig_args.ip->sval[0];
    const char * netmask_str = ifconfig_args.netmask->sval[0];
    const char * gateway_str = ifconfig_args.gateway->sval[0];
    const char * dns_str     = ifconfig_args.dns->sval[0];

    ip4_addr_t ip_addr = { 0 };
    ip4_addr_t netmask_addr = { 0 };
    ip4_addr_t gateway_addr = { 0 };
    ip4_addr_t dns_addr = { 0 };

    IPSTR_TO_ESPIP(ip_str,      ip_addr,      "Invalid IP address")
    IPSTR_TO_ESPIP(netmask_str, netmask_addr, "Invalid netmask")
    IPSTR_TO_ESPIP(gateway_str, gateway_addr, "Invalid gateway")
    IPSTR_TO_ESPIP(dns_str,     dns_addr,     "Invalid DNS")

    printf("  %-20s: %s\n", "IP Address", ip_str);
    printf("  %-20s: %s\n", "Netmask"   , netmask_str);
    printf("  %-20s: %s\n", "Gateway"   , gateway_str);
    printf("  %-20s: %s\n", "DNS"       , dns_str);

    printf("Setting manual configuration on interface %s\n", connection->get_name());
    connection->set_network_info(
        ip_addr.addr,
        netmask_addr.addr,
        gateway_addr.addr,
        dns_addr.addr
    );
    return 0;
}

//*****************************************************************************
static int print_interface_information_op(Connection *connection)
{
    if (!connection) {
        print_interface_information(MN8App::instance().get_wifi_connection());
        print_interface_information(MN8App::instance().get_ethernet_connection());
    } else {
        print_interface_information(connection);
    }
    return 0;
}

//*****************************************************************************
static int ifconfig_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &ifconfig_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, ifconfig_args.end, argv[0]);
        return 1;
    }
        
    Connection *connection = MN8App::instance().get_connection(ifconfig_args.interface->sval[0]);

    operation_t op = infer_operation_from_args();
    switch (op) {
        case e_print_interface_information_op: return print_interface_information_op(connection);
        case e_interface_up_op:            return op_interface_up(connection);
        case e_interface_down_op:          return op_interface_down(connection);
        case e_interface_dhcp_op:          return op_interface_dhcp(connection);
        case e_interface_manual_op:        return op_interface_manual(connection);
        case e_interface_reset_conf_op:    return op_interface_reset_conf(connection);
        case e_interface_dump_conf_op:     return op_interface_dump_conf(connection);
        case e_unknown_op:
            printf("Unknown operation\n");
            return 1;
    }

    return 0;
}

//*****************************************************************************
void register_ifconfig(void)
{
    ifconfig_args.interface = arg_str0(NULL, NULL, "<eth/wifi>", "Which interface to configure");
    ifconfig_args.command   = arg_str0(NULL, NULL, "<up/down/dhcp/manual/reset>", "Command to execute");
    ifconfig_args.ip        = arg_str0(NULL, NULL, "<ip>", "IP address");
    ifconfig_args.netmask   = arg_str0(NULL, NULL, "<netmask>", "Netmask");
    ifconfig_args.gateway   = arg_str0(NULL, NULL, "<gateway>", "Gateway");
    ifconfig_args.dns       = arg_str0(NULL, NULL, "<dns>", "DNS");
    ifconfig_args.end       = arg_end(6);

    #pragma GCC diagnostic pop "-Wmissing-field-initializers" 
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    const esp_console_cmd_t cmd = {
        .command = "ifconfig",
        .help = "Operate on network interfaces (wifi/ethernet)",
        .hint = NULL,
        .func = &ifconfig_cmd,
        .argtable = &ifconfig_args
    };
    #pragma GCC diagnostic push "-Wmissing-field-initializers" 
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}