// tests.cpp

#include <gtest/gtest.h>
#include "Led.h"
#include "ChargingAnimation.h"

TEST(animation, chargeLevel)
{
    ChargingAnimation testAnimate;

    //ASSERT_TRUE (testAnimate.get_charge_percent() == 0);

    testAnimate.set_charge_percent(50);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 50);

    testAnimate.set_charge_percent(0);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 0);

    testAnimate.set_charge_percent(100);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 100);

    testAnimate.set_charge_percent(0);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 0);

    // testAnimate.set_charge_percent(150);
    //ASSERT_TRUE (testAnimate.get_charge_percent() == 100);

    // Test after reset
    //testAnimate.reset(0,0,0,0);
    // ASSERT_TRUE (testAnimate.get_charge_percent() == 100);

}

TEST(animation, chargeLevelLeds)
{
    ChargingAnimation testAnimate;

    uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};

    // Remain agnostic towards algorithm, bud validate a few
    // invariants as we loop through charge percentages of 0-100.
    //
    // The "Charged LED" Count shoud:
    //   - Start at 1
    //   - Increases monotonically
    //   - All increases are by one (all levels must be visited)
    //   - Reaches but does not exceed LED_STRIP_PIXEL_COUNT

    uint32_t lastChargedLedCount = 0;
    for (int chargeLevel = 0; chargeLevel <= 100; chargeLevel++)
    {
        testAnimate.set_charge_percent(chargeLevel);
        testAnimate.refresh (led_pixels, LED_STRIP_PIXEL_COUNT, 0);
        uint32_t chargedLedCount = testAnimate.get_charged_led_count();

        printf ("utest: Charge pct of %d: chargedLED:%d (previous: %d)\n", chargeLevel, chargedLedCount, lastChargedLedCount);

// For now, just printing things out; assertions to be added later
#if 0
        // Always at least one; never more than max
        ASSERT_TRUE (chargedLedCount >= 1);
        ASSERT_TRUE (chargedLedCount <= LED_STRIP_PIXEL_COUNT);

        // Always at least previous level
        ASSERT_TRUE (chargedLedCount >= lastChargedLedCount);

        // Never one more than last
        ASSERT_TRUE (lastChargedLedCount -  chargedLedCount < 1);
#endif
        // Save for next iteration
        lastChargedLedCount = chargedLedCount;
    }
}
 
TEST(animation, pixels)
{

    ChargingAnimation testAnimate;

    uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};

    ASSERT_TRUE (led_pixels[0] == 0); 

    // GRB
    testAnimate.reset(0x0000FF, 0x0000FF, 0x000000, 10);
    testAnimate.refresh(led_pixels, LED_STRIP_PIXEL_COUNT, 0);

    testAnimate.set_charge_percent (100);

    testAnimate.refresh(led_pixels, LED_STRIP_PIXEL_COUNT, 0);

    //ASSERT_TRUE (led_pixels[2] != 0);

};

 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
