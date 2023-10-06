// tests.cpp

#include <gtest/gtest.h>
#include "ChargingAnimation.h"

TEST(Charging, TestOne) 
{
    ChargingAnimation testAnimate;

    testAnimate.set_charge_percent(50);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 50);

#if 0
    testAnimate.set_charge_percent(150);
    ASSERT_TRUE (testAnimate.get_charge_percent() != 150);
#endif

}
 
TEST(AnimationTest, Base) {

    ChargingAnimation testAnimate;

    uint8_t led_pixels[3*33] = {0};

    ASSERT_TRUE (led_pixels[0] == 0); 

    // GRB
    testAnimate.reset(0x0000FF, 0x0000FF, 0x000000, 10);
    testAnimate.refresh(led_pixels, 33, 0); 

    testAnimate.set_charge_percent (50); 

    testAnimate.refresh(led_pixels, 33, 0); 

    ASSERT_TRUE (led_pixels[2] != 0); 

};

 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
