// references:
// https://mischianti.org/esp32-ota-update-with-web-browser-firmware-filesystem-and-authentication-1/#google_vignette

#include "web_server.h"
#include "esp_log.h"
#include "esp_err.h"
#include "LED/Animations/ChargingAnimation.h"
#include "Utils/Updater.h"

static const char *TAG = "web_server";

static const char serverIndex2[] =
R"(<!DOCTYPE html>
     <html lang='en'>
     <head>
         <meta charset='utf-8'>
         <meta name='viewport' content='width=device-width,initial-scale=1'/>
     </head>
     <body>
     <form method='POST' action='' enctype='multipart/form-data'>
         Firmware:<br>
         <input type='file' accept='.bin,.bin.gz' name='firmware'>
         <input type='submit' value='Update Firmware'>
     </form>
     <form method='POST' action='' enctype='multipart/form-data'>
         FileSystem:<br>
         <input type='file' accept='.bin,.bin.gz,.image' name='filesystem'>
         <input type='submit' value='Update FileSystem'>
     </form>
     </body>
     </html>)";

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req)
{
    /* Send a simple response */
    httpd_resp_send(req, serverIndex2, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Our URI handler function to be called during POST /uri request */
esp_err_t post_handler(httpd_req_t *req)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[100];

    ESP_LOGI(TAG, "Content length: %d", req->content_len);
    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    { /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    /* Send a simple response */
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

#define ONE_KB (1024)
#define ONE_MB (ONE_KB)
uint8_t update_buffer[16*ONE_KB];

esp_err_t update_handler(httpd_req_t *req)
{
    char* ptr = (char*) update_buffer;
    esp_err_t ret = ESP_OK;
    bool done = false;
    
    uint32_t total_size = req->content_len;
    uint32_t left_to_recv = req->content_len;
    uint32_t bytes_written = 0;

    FwUpdater& updater = FwUpdater::instance();

    ret = updater.begin(total_size);
    if ( ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to begin update");
        goto err;
    }

    ESP_LOGI(TAG, "Content length: %ld", total_size);
    while (bytes_written < total_size) {
        size_t to_recv = MIN(left_to_recv, sizeof(update_buffer));
        int ret = httpd_req_recv(req, ptr, to_recv);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "Socket timeout, retrying");
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }

        updater.write(update_buffer, ret);

        left_to_recv -= ret;
        bytes_written += ret;
    }
    ESP_LOGI(TAG, "bytes_written: %ld", bytes_written);

    ret = updater.end();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to end update");
        goto err;
    }

    {
        const char resp[] = "OK";
        ESP_LOGI(TAG, "Update complete");
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

err:
    const char resp[] = "Failed";
    updater.abort();
    ESP_LOGI(TAG, "Update failed");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ret;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri = "/update",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL};

httpd_uri_t uri_update = {
    .uri = "/update",
    .method = HTTP_POST,
    .handler = update_handler,
    .user_ctx = NULL};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK)
    {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_update);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server)
    {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}

// /* Simple HTTP Server Example

//    This example code is in the Public Domain (or CC0 licensed, at your option.)

//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */

// #include <esp_wifi.h>
// #include <esp_event.h>
// #include <esp_log.h>
// #include <esp_system.h>
// #include <nvs_flash.h>
// #include <sys/param.h>
// #include "nvs_flash.h"
// #include "esp_netif.h"
// #include "esp_eth.h"
// #include "protocol_examples_common.h"
// #include "esp_tls_crypto.h"
// #include <esp_http_server.h>

// /* A simple example that demonstrates how to create GET and POST
//  * handlers for the web server.
//  */

// static const char *TAG = "example";

// #if CONFIG_EXAMPLE_BASIC_AUTH

// typedef struct
// {
//     char *username;
//     char *password;
// } basic_auth_info_t;

// #define HTTPD_401 "401 UNAUTHORIZED" /*!< HTTP Response 401 */

// static char *http_auth_basic(const char *username, const char *password)
// {
//     int out;
//     char *user_info = NULL;
//     char *digest = NULL;
//     size_t n = 0;
//     asprintf(&user_info, "%s:%s", username, password);
//     if (!user_info)
//     {
//         ESP_LOGE(TAG, "No enough memory for user information");
//         return NULL;
//     }
//     esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

//     /* 6: The length of the "Basic " string
//      * n: Number of bytes for a base64 encode format
//      * 1: Number of bytes for a reserved which be used to fill zero
//      */
//     digest = calloc(1, 6 + n + 1);
//     if (digest)
//     {
//         strcpy(digest, "Basic ");
//         esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
//     }
//     free(user_info);
//     return digest;
// }

// /* An HTTP GET handler */
// static esp_err_t basic_auth_get_handler(httpd_req_t *req)
// {
//     char *buf = NULL;
//     size_t buf_len = 0;
//     basic_auth_info_t *basic_auth_info = req->user_ctx;

//     buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
//     if (buf_len > 1)
//     {
//         buf = calloc(1, buf_len);
//         if (!buf)
//         {
//             ESP_LOGE(TAG, "No enough memory for basic authorization");
//             return ESP_ERR_NO_MEM;
//         }

//         if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK)
//         {
//             ESP_LOGI(TAG, "Found header => Authorization: %s", buf);
//         }
//         else
//         {
//             ESP_LOGE(TAG, "No auth value received");
//         }

//         char *auth_credentials = http_auth_basic(basic_auth_info->username, basic_auth_info->password);
//         if (!auth_credentials)
//         {
//             ESP_LOGE(TAG, "No enough memory for basic authorization credentials");
//             free(buf);
//             return ESP_ERR_NO_MEM;
//         }

//         if (strncmp(auth_credentials, buf, buf_len))
//         {
//             ESP_LOGE(TAG, "Not authenticated");
//             httpd_resp_set_status(req, HTTPD_401);
//             httpd_resp_set_type(req, "application/json");
//             httpd_resp_set_hdr(req, "Connection", "keep-alive");
//             httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
//             httpd_resp_send(req, NULL, 0);
//         }
//         else
//         {
//             ESP_LOGI(TAG, "Authenticated!");
//             char *basic_auth_resp = NULL;
//             httpd_resp_set_status(req, HTTPD_200);
//             httpd_resp_set_type(req, "application/json");
//             httpd_resp_set_hdr(req, "Connection", "keep-alive");
//             asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", basic_auth_info->username);
//             if (!basic_auth_resp)
//             {
//                 ESP_LOGE(TAG, "No enough memory for basic authorization response");
//                 free(auth_credentials);
//                 free(buf);
//                 return ESP_ERR_NO_MEM;
//             }
//             httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
//             free(basic_auth_resp);
//         }
//         free(auth_credentials);
//         free(buf);
//     }
//     else
//     {
//         ESP_LOGE(TAG, "No auth header received");
//         httpd_resp_set_status(req, HTTPD_401);
//         httpd_resp_set_type(req, "application/json");
//         httpd_resp_set_hdr(req, "Connection", "keep-alive");
//         httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
//         httpd_resp_send(req, NULL, 0);
//     }

//     return ESP_OK;
// }

// static httpd_uri_t basic_auth = {
//     .uri = "/basic_auth",
//     .method = HTTP_GET,
//     .handler = basic_auth_get_handler,
// };

// static void httpd_register_basic_auth(httpd_handle_t server)
// {
//     basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
//     if (basic_auth_info)
//     {
//         basic_auth_info->username = CONFIG_EXAMPLE_BASIC_AUTH_USERNAME;
//         basic_auth_info->password = CONFIG_EXAMPLE_BASIC_AUTH_PASSWORD;

//         basic_auth.user_ctx = basic_auth_info;
//         httpd_register_uri_handler(server, &basic_auth);
//     }
// }
// #endif

// /* An HTTP GET handler */
// static esp_err_t hello_get_handler(httpd_req_t *req)
// {
//     char *buf;
//     size_t buf_len;

//     /* Get header value string length and allocate memory for length + 1,
//      * extra byte for null termination */
//     buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
//     if (buf_len > 1)
//     {
//         buf = malloc(buf_len);
//         /* Copy null terminated value string into buffer */
//         if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
//         {
//             ESP_LOGI(TAG, "Found header => Host: %s", buf);
//         }
//         free(buf);
//     }

//     buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
//     if (buf_len > 1)
//     {
//         buf = malloc(buf_len);
//         if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK)
//         {
//             ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
//         }
//         free(buf);
//     }

//     buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
//     if (buf_len > 1)
//     {
//         buf = malloc(buf_len);
//         if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK)
//         {
//             ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
//         }
//         free(buf);
//     }

//     /* Read URL query string length and allocate memory for length + 1,
//      * extra byte for null termination */
//     buf_len = httpd_req_get_url_query_len(req) + 1;
//     if (buf_len > 1)
//     {
//         buf = malloc(buf_len);
//         if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
//         {
//             ESP_LOGI(TAG, "Found URL query => %s", buf);
//             char param[32];
//             /* Get value of expected key from query string */
//             if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK)
//             {
//                 ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
//             }
//             if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK)
//             {
//                 ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
//             }
//             if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK)
//             {
//                 ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
//             }
//         }
//         free(buf);
//     }

//     /* Set some custom headers */
//     httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
//     httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

//     /* Send response with custom headers and body set as the
//      * string passed in user context*/
//     const char *resp_str = (const char *)req->user_ctx;
//     httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

//     /* After sending the HTTP response the old HTTP request
//      * headers are lost. Check if HTTP request headers can be read now. */
//     if (httpd_req_get_hdr_value_len(req, "Host") == 0)
//     {
//         ESP_LOGI(TAG, "Request headers lost");
//     }
//     return ESP_OK;
// }

// static const httpd_uri_t hello = {
//     .uri = "/hello",
//     .method = HTTP_GET,
//     .handler = hello_get_handler,
//     /* Let's pass response string in user
//      * context to demonstrate it's usage */
//     .user_ctx = "Hello World!"};

// /* An HTTP POST handler */
// static esp_err_t echo_post_handler(httpd_req_t *req)
// {
//     char buf[100];
//     int ret, remaining = req->content_len;

//     while (remaining > 0)
//     {
//         /* Read the data for the request */
//         if ((ret = httpd_req_recv(req, buf,
//                                   MIN(remaining, sizeof(buf)))) <= 0)
//         {
//             if (ret == HTTPD_SOCK_ERR_TIMEOUT)
//             {
//                 /* Retry receiving if timeout occurred */
//                 continue;
//             }
//             return ESP_FAIL;
//         }

//         /* Send back the same data */
//         httpd_resp_send_chunk(req, buf, ret);
//         remaining -= ret;

//         /* Log data received */
//         ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
//         ESP_LOGI(TAG, "%.*s", ret, buf);
//         ESP_LOGI(TAG, "====================================");
//     }

//     // End response
//     httpd_resp_send_chunk(req, NULL, 0);
//     return ESP_OK;
// }

// static const httpd_uri_t echo = {
//     .uri = "/echo",
//     .method = HTTP_POST,
//     .handler = echo_post_handler,
//     .user_ctx = NULL};

// /* This handler allows the custom error handling functionality to be
//  * tested from client side. For that, when a PUT request 0 is sent to
//  * URI /ctrl, the /hello and /echo URIs are unregistered and following
//  * custom error handler http_404_error_handler() is registered.
//  * Afterwards, when /hello or /echo is requested, this custom error
//  * handler is invoked which, after sending an error message to client,
//  * either closes the underlying socket (when requested URI is /echo)
//  * or keeps it open (when requested URI is /hello). This allows the
//  * client to infer if the custom error handler is functioning as expected
//  * by observing the socket state.
//  */
// esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
// {
//     if (strcmp("/hello", req->uri) == 0)
//     {
//         httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
//         /* Return ESP_OK to keep underlying socket open */
//         return ESP_OK;
//     }
//     else if (strcmp("/echo", req->uri) == 0)
//     {
//         httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
//         /* Return ESP_FAIL to close underlying socket */
//         return ESP_FAIL;
//     }
//     /* For any other URI send 404 and close socket */
//     httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
//     return ESP_FAIL;
// }

// /* An HTTP PUT handler. This demonstrates realtime
//  * registration and deregistration of URI handlers
//  */
// static esp_err_t ctrl_put_handler(httpd_req_t *req)
// {
//     char buf;
//     int ret;

//     if ((ret = httpd_req_recv(req, &buf, 1)) <= 0)
//     {
//         if (ret == HTTPD_SOCK_ERR_TIMEOUT)
//         {
//             httpd_resp_send_408(req);
//         }
//         return ESP_FAIL;
//     }

//     if (buf == '0')
//     {
//         /* URI handlers can be unregistered using the uri string */
//         ESP_LOGI(TAG, "Unregistering /hello and /echo URIs");
//         httpd_unregister_uri(req->handle, "/hello");
//         httpd_unregister_uri(req->handle, "/echo");
//         /* Register the custom error handler */
//         httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
//     }
//     else
//     {
//         ESP_LOGI(TAG, "Registering /hello and /echo URIs");
//         httpd_register_uri_handler(req->handle, &hello);
//         httpd_register_uri_handler(req->handle, &echo);
//         /* Unregister custom error handler */
//         httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
//     }

//     /* Respond with empty body */
//     httpd_resp_send(req, NULL, 0);
//     return ESP_OK;
// }

// static const httpd_uri_t ctrl = {
//     .uri = "/ctrl",
//     .method = HTTP_PUT,
//     .handler = ctrl_put_handler,
//     .user_ctx = NULL};

// static httpd_handle_t start_webserver(void)
// {
//     httpd_handle_t server = NULL;
//     httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//     config.lru_purge_enable = true;

//     // Start the httpd server
//     ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
//     if (httpd_start(&server, &config) == ESP_OK)
//     {
//         // Set URI handlers
//         ESP_LOGI(TAG, "Registering URI handlers");
//         httpd_register_uri_handler(server, &hello);
//         httpd_register_uri_handler(server, &echo);
//         httpd_register_uri_handler(server, &ctrl);
// #if CONFIG_EXAMPLE_BASIC_AUTH
//         httpd_register_basic_auth(server);
// #endif
//         return server;
//     }

//     ESP_LOGI(TAG, "Error starting server!");
//     return NULL;
// }

// static esp_err_t stop_webserver(httpd_handle_t server)
// {
//     // Stop the httpd server
//     return httpd_stop(server);
// }

// static void disconnect_handler(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data)
// {
//     httpd_handle_t *server = (httpd_handle_t *)arg;
//     if (*server)
//     {
//         ESP_LOGI(TAG, "Stopping webserver");
//         if (stop_webserver(*server) == ESP_OK)
//         {
//             *server = NULL;
//         }
//         else
//         {
//             ESP_LOGE(TAG, "Failed to stop http server");
//         }
//     }
// }

// static void connect_handler(void *arg, esp_event_base_t event_base,
//                             int32_t event_id, void *event_data)
// {
//     httpd_handle_t *server = (httpd_handle_t *)arg;
//     if (*server == NULL)
//     {
//         ESP_LOGI(TAG, "Starting webserver");
//         *server = start_webserver();
//     }
// }

// // void app_main(void)
// // {
// //     static httpd_handle_t server = NULL;

// //     ESP_ERROR_CHECK(nvs_flash_init());
// //     ESP_ERROR_CHECK(esp_netif_init());
// //     ESP_ERROR_CHECK(esp_event_loop_create_default());

// //     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
// //      * Read "Establishing Wi-Fi or Ethernet Connection" section in
// //      * examples/protocols/README.md for more information about this function.
// //      */
// //     ESP_ERROR_CHECK(example_connect());

// //     /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
// //      * and re-start it upon connection.
// //      */
// // #ifdef CONFIG_EXAMPLE_CONNECT_WIFI
// //     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
// //     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
// // #endif // CONFIG_EXAMPLE_CONNECT_WIFI
// // #ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
// //     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
// //     ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
// // #endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

// //     /* Start the server for the first time */
// //     server = start_webserver();
// // }
