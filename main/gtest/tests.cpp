
#include <stdlib.h>
#include <limits.h>
#include <arpa/inet.h>


#include <gtest/gtest.h>
#include "Led.h"
#include "ChargingAnimation.h"
#include "Utils/Colors.h"

/* Helper function prototypes */
static int blueLedCount (uint8_t *pLedPixels);
static void showLed (uint8_t *pLedPixels, int ledIndex);
static void showLedStripHeader(void);

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
        uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};
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
 * @brief Test animated display at a few charge levels.
 *
 *        Counts total number of blue pixels on the LED bar.
 *        This is not as throrough as a full pixel-by-pixel check
 *        but still effective and easy to implement.
 *
 */
TEST(animation, chargeLevelLedsDynamic)
{
    ChargingAnimation testAnimate;
    ASSERT_FALSE (testAnimate.get_charge_simulation());

    // Start charge level high enough that we expect three LEDs
    // to be blue (base indicator and charge level indicator)
    for (int chargeLevel = 30; chargeLevel <= 100; chargeLevel++)
    {
        ChargingAnimation testAnimate;
        ASSERT_FALSE (testAnimate.get_charge_simulation());
        testAnimate.set_quiet(true);

        uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};

        testAnimate.reset(COLOR_BLUE, COLOR_WHITE, COLOR_BLUE, 100);
        testAnimate.set_charge_percent(chargeLevel);
        testAnimate.refresh (led_pixels, LED_STRIP_PIXEL_COUNT, 0);

        // Make sure we are starting with normal blue LED count (usually 3)
        int baseBlueCnt = CHARGE_BASE_LED_CNT + CHARGE_LEVEL_LED_CNT;
        ASSERT_TRUE (blueLedCount (led_pixels) == baseBlueCnt);

        int lastBlueCnt = baseBlueCnt;
        for (int cycles = 0; cycles < 100; cycles++)
        {
            testAnimate.refresh (led_pixels, LED_STRIP_PIXEL_COUNT, 0);
            // Should have one more blue pixel than last time, or it's cycled
            // back to base (3 LEDs).
            int blueCnt = blueLedCount (led_pixels);

            ASSERT_TRUE (blueCnt == lastBlueCnt+1 || blueCnt == baseBlueCnt);

            lastBlueCnt = blueCnt;
        }
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
 * @brief  Count number of BLUE LEDs in an array. Helper function for
 *         testing animation.
 */
static int blueLedCount (uint8_t *pLedPixels)
{
    int blueCount = 0;

    if (pLedPixels != NULL)
    {
        for (int x = 0; x < LED_STRIP_PIXEL_COUNT; x++)
        {
            if ((pLedPixels[(3 * x) + 0] == 0x00) &&
                (pLedPixels[(3 * x) + 1] == 0x00) &&
                (pLedPixels[(3 * x) + 2] == 0xFF))
            {
                blueCount++;
            }
        }
    }

    //printf ("Counted %d blue LEDs\n", blueCount);
    return blueCount;
}





//******************************************************************************
/**
 * @brief Mostly for interactive testing, shows an ASCII representation 
 *        of a passed LED/Pixel.  
 */
static void showLed (uint8_t *pLedPixels, int ledIndex)
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
static void showLedStripHeader(void)
{
    printf ("LED BAR:                      1 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 3 3\n");
    printf ("LED BAR:  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n");
}

//******************************************************************************
/**
 * @brief Basic ASCII representation of LED bar.
 */
static void showLedStrip (uint8_t *pLedPixels)
{

    printf ("LED BAR: ");
    for (int led=0; led < LED_STRIP_PIXEL_COUNT; led++)
    {
        showLed(pLedPixels, led);
    }
    printf ("|\n");
}


//******************************************************************************
/**
 * @brief Allows unit test runners to specify a charge level via command line 
 *        and see a (crude) display of "animation". Covid-driven test (no access 
 *        to an LED bar).
 */
void interactive_mode (void)
{
    char *string = NULL;
    size_t size = 0;
    ssize_t chars_read;

    // read a long string with getline

    while (true)
    {

        printf ("\nEnter an option:\n"
                "  <num> to animate charge level (0-100)\n"
                "  +<num> to bump charge level 5%% during animation\n"
                "  -<num> to cut charge level 5%% during animation\n"
                " 'q' or 'x' to quit\n"
                " 's' to run charge simulation (long!)\n"
                "led_test> ");

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

        if (tolower(string[0]) == 's')
        {
            // Simulated charge
            ChargingAnimation testAnimate;
            testAnimate.reset(COLOR_BLUE, COLOR_WHITE, COLOR_BLUE, 100);
            uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};
            testAnimate.set_quiet (true);
            testAnimate.set_charge_simulation(true);

            showLedStripHeader();
            int numCycles = 1100;
            for (int c = 0; c < numCycles; c++)
            {
                testAnimate.refresh(led_pixels, LED_STRIP_PIXEL_COUNT, 0);
                showLedStrip (led_pixels);
            }

        }
        else
        {

            // User enters a charge percentage; simulated animation
            // is displayed.

            // Option to change charge level during animation
            bool bumpDuringAnimate = false;
            bool cutDuringAnimate = false;

            // Option to bump charge level during animation
            if (tolower(string[0]) == '+')
            {
                bumpDuringAnimate = true;
                string[0] = ' ';
            }
            if (tolower(string[0]) == '-')
            {
                cutDuringAnimate = true;
                string[0] = ' ';
            }


            long chargePct = strtol (string, NULL, 10);
            if (chargePct == LONG_MIN)
            {
                printf ("Input error");
                return;
            }

            printf ("Animation of charge pct: %ld (bump during animate: %s)\n", chargePct, bumpDuringAnimate ? "yes":"no");

            ChargingAnimation testAnimate;
            testAnimate.set_quiet (true);

            uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};
            testAnimate.reset(COLOR_BLUE, COLOR_WHITE, COLOR_BLUE, 100);

            testAnimate.set_charge_percent(chargePct);

            // Try to show one full cycle of charge animation, but where possible
            // keep it on the same "page". Roughly the number of animated pixels
            // plus a few more to show the wrap-around.
            int numCycles = ((chargePct * LED_STRIP_PIXEL_COUNT) / 100) + 3;
            showLedStripHeader();
            for (int c = 0; c < numCycles; c++)
            {
                if (c == numCycles/2)
                {
                    // roughly haflway through. Adjust charge level
                    // if requested.
                    if (bumpDuringAnimate)
                    {
                        testAnimate.set_charge_percent(chargePct+5);
                    }
                    if (cutDuringAnimate)
                    {
                        testAnimate.set_charge_percent(chargePct-5);
                    }
                }

                testAnimate.refresh(led_pixels, LED_STRIP_PIXEL_COUNT, 0);
                showLedStrip (led_pixels);
            }
        }
    }

    return;

}
 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

    // See if interactive/demo mode is selected; if not just
    // run unit tests normally (RUN_ALL_TESTS)

    bool interactive=false;
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
        printf ("INTERACTIVE MODE:\n");
        interactive_mode();
    }
}
