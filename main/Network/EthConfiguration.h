#pragma once

#include "NetworkConfiguration.h"

class EthernetConfiguration : public NetworkConfiguration {
public:
    EthernetConfiguration() {};
    ~EthernetConfiguration(void) = default;

public:

protected:
    virtual const char * get_store_section_name(void) override { return "eth"; }
    

private:
};