#include "mdns_broadcaster.h"

#include "utils/FuseMacAddress.h"

#include "esp_netif_ip_addr.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mdns.h"
#include "netdb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char* TAG = "mdns_broadcaster";

#define CONFIG_MDNS_HOSTNAME "mn8-lightbar-mdns"
#define CONFIG_MDNS_INSTANCE "MN8 lightbar"

static char *generate_hostname(void);

esp_err_t initialise_mdns(void) {
    char *hostname = generate_hostname();

    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);
    //set default mDNS instance name
    ESP_ERROR_CHECK( mdns_instance_name_set(CONFIG_MDNS_INSTANCE) );

    //structure with TXT records
    uint8_t mac[6];
    get_fuse_mac_address(mac);
    char mac_str[18];
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);

    mdns_txt_item_t serviceTxtData[1] = {
        {"mac", mac_str}
    };

    ESP_ERROR_CHECK( mdns_service_add("MN8-lightbar-Control-Interface", "_mn8", "_udp", 24242, serviceTxtData, 1) );

    ESP_LOGI(TAG, "mdns service added: [%s]", "MN8-lightbar-Control-Interface");
    //add another TXT item
    //ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "path", "/foobar") );
    //change TXT item value
    //ESP_ERROR_CHECK( mdns_service_txt_item_set_with_explicit_value_len("_http", "_tcp", "u", "admin", strlen("admin")) );
    free(hostname);
    return ESP_OK;
}

static char *generate_hostname(void)
{
//    return strdup(CONFIG_MDNS_HOSTNAME);

    char   *hostname;
    uint8_t mac[6];
    get_fuse_mac_address(mac);

    if (-1 == asprintf(&hostname, "%s-%02X%02X%02X%02X%02X%02X", CONFIG_MDNS_HOSTNAME, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5])) {
        abort();
    }
    return hostname;
}