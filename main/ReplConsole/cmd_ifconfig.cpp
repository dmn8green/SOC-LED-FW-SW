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

#include "Network/Connection.h"
#include "MN8App.h"

// ifconfig interface up/down
// ifconfig interface ip, netmask, gateway

static struct {
    struct arg_str *interface;
    struct arg_str *command;
    struct arg_str *ip;
    struct arg_str *netmask;
    struct arg_str *gateway;
    struct arg_end *end;
} ifconfig_args;

#define STR_IS_EQUAL(str1, str2) (strncasecmp(str1, str2, strlen(str2)) == 0)
#define STR_NOT_EMPTY(str) (strlen(str) > 0)
#define STR_IS_EMPTY(str) (strlen(str) == 0)

static void print_interface_information(Connection *connection)
{
    printf("\nInterface information:\n");
    printf("  %-20s: %s\n", "Interface", connection->get_name());

    printf("  %-20s: %s\n", "Enabled", connection->is_enabled() ? "Yes" : "No");

    if (connection->is_enabled()) {
        printf("  %-20s: %s\n", "Connected", connection->is_connected() ? "Yes" : "No");
        printf("  %-20s: %s\n", "DHCP", connection->is_dhcp() ? "Yes" : "No");
    }

    if (connection->is_connected()) {
        esp_ip4_addr_t ip = connection->get_ip_address();
        esp_ip4_addr_t netmask = connection->get_netmask();
        esp_ip4_addr_t gateway = connection->get_gateway();
        
        printf("  %-20s: " IPSTR "\n", "IP Address", IP2STR(&ip));
        printf("  %-20s: " IPSTR "\n", "Netmask", IP2STR(&netmask));
        printf("  %-20s: " IPSTR "\n", "Gateway", IP2STR(&gateway));

        printf("  %-20s: %08lx\n", "IP Address", ip.addr);
        printf("  %-20s: %08lx\n", "Netmask", netmask.addr);
        printf("  %-20s: %08lx\n", "Gateway", gateway.addr);
    }
    printf("~~~~~~~~~~~\n");
}

typedef enum {
    e_print_interface_information_op,
    e_set_interface_up_op,
    e_set_interface_down_op,
    e_set_interface_enable_op,
    e_set_interface_disable_op,
    e_set_interface_dhcp_op,
    e_set_interface_manual_op,
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
    if (STR_IS_EQUAL(cmd, "enable") || STR_IS_EQUAL(cmd, "disable")) {
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

    if (STR_IS_EQUAL(command, "up"))   { return e_set_interface_up_op; }
    if (STR_IS_EQUAL(command, "down")) { return e_set_interface_down_op; }
    if (STR_IS_EQUAL(command, "dhcp")) { return e_set_interface_dhcp_op; }
    if (STR_IS_EQUAL(command, "enable")) { return e_set_interface_enable_op; }
    if (STR_IS_EQUAL(command, "disable")) { return e_set_interface_disable_op; }

    if (STR_IS_EQUAL(command, "manual")) {
        if (STR_IS_EMPTY(ip) || STR_IS_EMPTY(netmask) || STR_IS_EMPTY(gateway)) {
            printf("ERROR: IP, netmask and gateway must be specified for manual configuration");
            return e_unknown_op;
        }

        return e_set_interface_manual_op;
    }

    return e_unknown_op;
}

//*****************************************************************************
static int op_set_interface_up(Connection *connection)
{
    printf("Turning on interface %s\n", connection->get_name());
    return (connection->on() == ESP_OK) ? 0 : 1;
}

//*****************************************************************************
static int op_set_interface_down(Connection *connection)
{
    printf("Turning off interface %s\n", connection->get_name());
    return (connection->off() == ESP_OK) ? 0 : 1;
}

//*****************************************************************************
static int op_set_interface_enable(Connection *connection)
{
    printf("Enabling interface %s\n", connection->get_name());
    return (connection->set_enabled(true) == ESP_OK) ? 0 : 1;
}

//*****************************************************************************
static int op_set_interface_disable(Connection *connection)
{
    printf("Disabling interface %s\n", connection->get_name());
    return (connection->set_enabled(false) == ESP_OK) ? 0 : 1;
}

//*****************************************************************************
static int op_set_interface_dhcp(Connection *connection)
{
    printf("Turning on dhcp on interface %s\n", connection->get_name());
    return (connection->use_dhcp(true) == ESP_OK) ? 0 : 1;
}

int
ip4addr1_aton(const char *cp, ip4_addr_t *addr)
{
  u32_t val;
  u8_t base;
  char c;
  u32_t parts[4];
  u32_t *pp = parts;

  c = *cp;
  for (;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, 1-9=decimal.
     */
    if (!lwip_isdigit(c)) {
      return 0;
    }
    val = 0;
    base = 10;
    if (c == '0') {
      c = *++cp;
      if (c == 'x' || c == 'X') {
        base = 16;
        c = *++cp;
      } else {
        base = 8;
      }
    }
    for (;;) {
      if (lwip_isdigit(c)) {
        if((base == 8) && ((u32_t)(c - '0') >= 8))
          break;
        val = (val * base) + (u32_t)(c - '0');
        c = *++cp;
      } else if (base == 16 && lwip_isxdigit(c)) {
        val = (val << 4) | (u32_t)(c + 10 - (lwip_islower(c) ? 'a' : 'A'));
        c = *++cp;
      } else {
        break;
      }
    }
    if (c == '.') {
      /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c   (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       */
      if (pp >= parts + 3) {
        return 0;
      }
      *pp++ = val;
      c = *++cp;
    } else {
      break;
    }
  }
  /*
   * Check for trailing characters.
   */
  if (c != '\0' && !lwip_isspace(c)) {
    return 0;
  }
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  switch (pp - parts + 1) {

    case 0:
      return 0;       /* initial nondigit */

    case 1:             /* a -- 32 bits */
      break;

    case 2:             /* a.b -- 8.24 bits */
      if (val > 0xffffffUL) {
        return 0;
      }
      if (parts[0] > 0xff) {
        return 0;
      }
      val |= parts[0] << 24;
      break;

    case 3:             /* a.b.c -- 8.8.16 bits */
      if (val > 0xffff) {
        return 0;
      }
      if ((parts[0] > 0xff) || (parts[1] > 0xff)) {
        return 0;
      }
      val |= (parts[0] << 24) | (parts[1] << 16);
      break;

    case 4:             /* a.b.c.d -- 8.8.8.8 bits */
      if (val > 0xff) {
        return 0;
      }
      if ((parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff)) {
        return 0;
      }
      val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
      break;
    default:
      LWIP_ASSERT("unhandled", 0);
      break;
  }
  if (addr) {
    ip4_addr_set_u32(addr, lwip_htonl(val));
  }
  return 1;
}


//*****************************************************************************
static int op_set_interface_manual(Connection *connection)
{
#define IPSTR_TO_ESPIP(ipstr, espip, msg) \
    if (ip4addr_aton(ipstr, &espip) == 0) { \
        printf(msg " %s\n", ipstr); \
        return 1; \
    } 

    const char * ip_str      = ifconfig_args.ip->sval[0];
    const char * netmask_str = ifconfig_args.netmask->sval[0];
    const char * gateway_str = ifconfig_args.gateway->sval[0];

    ip4_addr_t ip_addr = { 0 };
    ip4_addr_t netmask_addr = { 0 };
    ip4_addr_t gateway_addr = { 0 };

    IPSTR_TO_ESPIP(ip_str,      ip_addr,      "Invalid IP address")
    IPSTR_TO_ESPIP(netmask_str, netmask_addr, "Invalid netmask")
    IPSTR_TO_ESPIP(gateway_str, gateway_addr, "Invalid gateway")

    printf("  %-20s: %s\n", "IP Address", ip_str);
    printf("  %-20s: %s\n", "Netmask"   , netmask_str);
    printf("  %-20s: %s\n", "Gateway"   , gateway_str);

    printf("  %-20s: %08lx\n", "IP Address4", ip_addr.addr);
    printf("  %-20s: %08lx\n", "Netmask4"   , netmask_addr.addr);
    printf("  %-20s: %08lx\n", "Gateway4"   , gateway_addr.addr);


    printf("Setting manual configuration on interface %s\n", connection->get_name());
    connection->set_network_info(
        ip_addr.addr,
        netmask_addr.addr,
        gateway_addr.addr
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

    printf("ifconfig: interface=%s, command=%s, ip=%s, netmask=%s, gateway=%s\n",
        ifconfig_args.interface->sval[0],
        ifconfig_args.command->sval[0],
        ifconfig_args.ip->sval[0],
        ifconfig_args.netmask->sval[0],
        ifconfig_args.gateway->sval[0]);
        
    Connection *connection = MN8App::instance().get_connection(ifconfig_args.interface->sval[0]);

    operation_t op = infer_operation_from_args();
    switch (op) {
        case e_print_interface_information_op: return print_interface_information_op(connection);
        case e_set_interface_up_op:            return op_set_interface_up(connection);
        case e_set_interface_down_op:          return op_set_interface_down(connection);
        case e_set_interface_enable_op:        return op_set_interface_enable(connection);
        case e_set_interface_disable_op:       return op_set_interface_disable(connection);
        case e_set_interface_dhcp_op:          return op_set_interface_dhcp(connection);
        case e_set_interface_manual_op:        return op_set_interface_manual(connection);
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
    ifconfig_args.command   = arg_str0(NULL, NULL, "<up/down/dhcp/manual>", "Command to execute");
    ifconfig_args.ip        = arg_str0(NULL, NULL, "<ip>", "IP address");
    ifconfig_args.netmask   = arg_str0(NULL, NULL, "<netmask>", "Netmask");
    ifconfig_args.gateway   = arg_str0(NULL, NULL, "<gateway>", "Gateway");
    ifconfig_args.end       = arg_end(5);

    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    const esp_console_cmd_t cmd = {
        .command = "ifconfig",
        .help = "Get information about network interfaces (wifi/ethernet)",
        .hint = NULL,
        .func = &ifconfig_cmd,
        .argtable = &ifconfig_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}