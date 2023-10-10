


#include <cstdio>

// ESP_LOGD doesn't have trailing '\n'; add it here for readability
//

#define ESP_LOGD(TAG, FMT, ...) printf (FMT"\n", ##__VA_ARGS__)
#define ESP_LOGI(TAG, FMT, ...) printf (FMT"\n", ##__VA_ARGS__)
