#pragma once

#include <string.h>
#include <stdint.h>

#include "esp_err.h"

#include "Utils/NoCopy.h"

class ThingConfig : public NoCopy {
public:
    ThingConfig();
    ~ThingConfig();

    // Load/Save from NVS
    esp_err_t load(void);
    esp_err_t save(void);

    // Setter
    void set_thing_name(const char* thingName);
    void set_certificate_arn(const char* certificateArn);
    void set_certificate_id(const char* certificateId);
    void set_certificate_pem(const char* certificatePem);
    void set_private_key(const char* privateKey);
    void set_public_key(const char* publicKey);
    void set_endpoint_address(const char* endpointAddress);

    // inline getter
    inline const char* get_thing_name(void) { return this->thingName; }
    inline const char* get_certificate_arn(void) { return this->certificateArn; }
    inline const char* get_certificate_id(void) { return this->certificateId; }
    inline const char* get_certificate_pem(void) { return this->certificatePem; }
    inline const char* get_private_key(void) { return this->privateKey; }
    inline const char* get_public_key(void) { return this->publicKey; }
    inline const char* get_endpoint_address(void) { return this->endpointAddress; }

    inline bool is_configured(void) { return this->isConfigured; }


private:
    char *thingName = nullptr;
    char *certificateArn = nullptr;
    char *certificateId = nullptr;
    char *certificatePem = nullptr;
    char *privateKey = nullptr;
    char *publicKey = nullptr;
    char *endpointAddress = nullptr;
    bool isConfigured = false;
};