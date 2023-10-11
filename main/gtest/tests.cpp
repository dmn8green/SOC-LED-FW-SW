
#include <stdlib.h>
#include <limits.h>
#include <arpa/inet.h>


#include <gtest/gtest.h>
#include "Led.h"
#include "ChargingAnimation.h"
#include "Utils/Colors.h"

//******************************************************************************
/**
 * @brief Test get/set of charge level; range limits enforced, etc.  
 * 
 */

TEST(animation, chargeLevelGetSet)
{
    ChargingAnimation testAnimate;

    ASSERT_FALSE (testAnimate.get_charge_simulation());

    ASSERT_TRUE (testAnimate.get_charge_percent() == 0);

    testAnimate.set_charge_percent(50);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 50);

    testAnimate.set_charge_percent(0);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 0);

    testAnimate.set_charge_percent(100);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 100);

    testAnimate.set_charge_percent(0);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 0);

    testAnimate.set_charge_percent(150);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 100);

    // Test after reset
    testAnimate.reset(0,0,0,0);
    ASSERT_TRUE (testAnimate.get_charge_percent() == 100);

}

//******************************************************************************
/**
 * @brief Test initial display of charge at all levels from 0 to 100  
 * 
 */

TEST(animation, chargeLevelLedsStatic)
{
    ChargingAnimation testAnimate;

    ASSERT_FALSE (testAnimate.get_charge_simulation());

    uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};

    // Remain agnostic towards algorithm, but validate a few
    // invariants as we loop through charge percentages of 0-100.
    //
    // The "Charged LED" Count shall: 
    //   - Start at 1
    //   - Increase monotonically with charge level
    //   - Increase only by one (all levels must be visited)
    //   - Reaches but does not exceed LED_STRIP_PIXEL_COUNT

    uint32_t lastChargedLedCount = 0;
    for (int chargeLevel = 0; chargeLevel <= 100; chargeLevel++)
    {
        testAnimate.set_charge_percent(chargeLevel);
        testAnimate.refresh (led_pixels, LED_STRIP_PIXEL_COUNT, 0);
        uint32_t chargedLedCount = testAnimate.get_led_charge_top_leds();

        printf ("utest: Charge pct of %d: chargedLED:%d (previous: %d)\n", chargeLevel, chargedLedCount, lastChargedLedCount);

        // Always at least one; never more than max
        ASSERT_TRUE (chargedLedCount >= 1);
        ASSERT_TRUE (chargedLedCount <= LED_STRIP_PIXEL_COUNT);

        // Always at least previous level
        ASSERT_TRUE (chargedLedCount >= lastChargedLedCount);

        // Never one more than last (for this test; real life could go backwards)
        ASSERT_TRUE (chargedLedCount -  lastChargedLedCount <= 1);

        // Save for next iteration
        lastChargedLedCount = chargedLedCount;
    }
}

//******************************************************************************
/**
 * @brief  Check pixel fill levels for charge level (static)
 * 
 */
TEST(animation, chargeLevelPixels)
{
    ChargingAnimation testAnimate;
    ASSERT_FALSE (testAnimate.get_charge_simulation());

    uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};

    // Since we are testing pixels, need to setup colors
    testAnimate.reset(COLOR_BLUE, COLOR_WHITE, COLOR_BLUE, 100);

    // Just one mid-level charge for now
    testAnimate.set_charge_percent(50);
    testAnimate.refresh (led_pixels, LED_STRIP_PIXEL_COUNT, 0);

    // first LED blue
    ASSERT_TRUE (led_pixels[0] == 0x00);
    ASSERT_TRUE (led_pixels[1] == 0x00);
    ASSERT_TRUE (led_pixels[2] == 0xFF);

    // Second LED white
    ASSERT_TRUE (led_pixels[3] == 0x80);
    ASSERT_TRUE (led_pixels[4] == 0x80);
    ASSERT_TRUE (led_pixels[5] == 0x80);

    // Verify that LEDs up to charge level are non-zero. Other
    // tests validate that get_charged_led_count() is OK, so we
    // use it here.
    uint32_t chargeLeds = testAnimate.get_led_charge_top_leds();

    ASSERT_TRUE (chargeLeds > 1); 

    // Charge level BLUE shoud be nonzero 
    ASSERT_TRUE (led_pixels[3 * (chargeLeds-CHARGE_LEVEL_LED_CNT) + 2] != 0); 

}



//******************************************************************************
/**
 * @brief Mostly for interactive testing, shows an ASCII representation 
 *        of a passed LED/Pixel.  
 */
void showLED (uint8_t *pLedPixels, int ledIndex)
{
    uint8_t g = *(pLedPixels + (ledIndex*3) + 0);
    uint8_t r = *(pLedPixels + (ledIndex*3) + 1);
    uint8_t b = *(pLedPixels + (ledIndex*3) + 2);

    char pchar = '?';

    // Left pixel bar (in black)
    printf ("|");

    uint32_t color = r << 16 | g << 8 | b; 

    switch (color) 
    {
        case COLOR_WHITE: 
            printf("\033[37m");
            pchar = '.';
            break; 
        case COLOR_CYAN: 
            printf("\033[36m");
            pchar = 'C';
            break;
        case COLOR_PURPLE:  // Magenta
            printf("\033[35m");
            pchar = 'P';
            break; 
        case COLOR_YELLOW:
            printf("\033[33m");
            pchar = 'Y';
            break; 
        case COLOR_RED:
            printf("\033[31m");
            pchar = 'R';
            break;
        case COLOR_GREEN:
            printf("\033[32m");
            pchar = 'G';
            break; 
        case COLOR_BLUE:
            printf("\033[34m");
            pchar = 'B';
            break; 
        case COLOR_BLACK:
            pchar = ' '; 
            break; 
        default: 
            break;
    }

    printf ("%c", pchar);
    printf("\033[0m");
}

//******************************************************************************
/**
 * @brief When displayed the LED bar, a header to indicate offsets. Fixed at 32 
 *        for now (TODO:)  
 */
void showBarHeader(void)
{
    printf ("LED BAR:  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n");
}

//******************************************************************************
/**
 * @brief Basic ASCII representation of LED bar.
 */
void showBar (uint8_t *pLedPixels)
{

    printf ("LED BAR: ");
    for (int led=0; led < LED_STRIP_PIXEL_COUNT; led++)
    {
        showLED(pLedPixels, led);
    }
    printf ("|\n");
}


//******************************************************************************
/**
 * @brief Allows unit test runners to specify a charge level via command line 
 *        and see a (crude) display of "animation". Covid-driven test (no access 
 *        to an LED bar).
 */
void interactive_charge (void)
{
    char *string = NULL;
    size_t size = 0;
    ssize_t chars_read;

    // read a long string with getline

    while (true)
    {

        printf ("Enter charge level (0-100) or q to quit\n");
        chars_read = getline(&string, &size, stdin);

        if (chars_read < 0)
        {
            printf ("Input error");
            return;
        }

        if (tolower(string[0]) == 'q' || tolower(string[0]) == 'x')
        {
            printf ("\nexiting\n");
            return;
        }

        long chargePct = strtol (string, NULL, 10);
        if (chargePct == LONG_MIN)
        {
            printf ("Input error");
            return;
        }

        printf ("charge sim:  %ld\n", chargePct);

        ChargingAnimation testAnimate;
        testAnimate.set_quiet (true);

        uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};
        testAnimate.reset(COLOR_BLUE, COLOR_WHITE, COLOR_BLUE, 100);

        testAnimate.set_charge_percent(chargePct);

        // Try to show one full cycle of charge animation, but where possible 
        // keep it on the same "page". Roughly the number of animated pixels 
        // plus a few more to show the wrap-around. 
        int numCycles = ((chargePct * LED_STRIP_PIXEL_COUNT) / 100) + 3;
        showBarHeader();
        for (int c = 0; c < numCycles; c++)
        {
            testAnimate.refresh(led_pixels, LED_STRIP_PIXEL_COUNT, 0);
            showBar(led_pixels);
        }
    }

    return;

}
 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    bool interactive=false;

    // See if interactive charging mode is selected

    int c;
    while ((c = getopt (argc, argv, "i")) != -1)
    switch (c)
    {
        case 'i':
        interactive = true;
    }

    if (!interactive)
    {
        return RUN_ALL_TESTS();
    }
    else
    {
        printf ("INTERACTIVE!\n");
        interactive_charge();
    }
}
