#pragma once

#include "Utils/Singleton.h"

class Time : public Singleton<Time>
{
public:
    Time(token) {}
    
    inline uint64_t now() { return upTimeMicro(); }
    inline uint64_t upTimeMicro() { return esp_timer_get_time(); }
    inline uint64_t upTimeMS() { return esp_timer_get_time() / 1000; }
    inline uint64_t upTimeS() { return esp_timer_get_time() / 1000000; }
    inline uint64_t upTimeM() { return esp_timer_get_time() / 1000000000; }
};