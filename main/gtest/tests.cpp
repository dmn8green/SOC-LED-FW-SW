
//******************************************************************************
/**
 * @file tests.cpp
 * @author bill mcguinness (bmcguinness@appliedlogix.com)
 *
 * @brief Unit testing for LED strip
 * @version 0.1
 * @date 2023-19-19
 *
 * @copyright Copyright MN8 (c) 2023
 *
 * Unit tests for LED Strip. With no parameters, runs normal unit tests with
 * pass/fail satus.
 *
 * When passed '-i' option, runs in interactive mode allowing users to see
 * animation patterns faster than in real-time (does not yet support 'pusling'
 * animations).
 *
 * In this test, each LED is really a controller which in the real world may
 * drive multiple LEDs. Our initial product has 31 or 32 controllers, each
 * driving three LEDs.
 *
 */
//******************************************************************************


#include <stdlib.h>
#include <limits.h>
#include <arpa/inet.h>


#include <gtest/gtest.h>
#include "Led.h"
#include "ChargingAnimation.h"
#include "Utils/Colors.h"

/* Helper function prototypes */
static int blueLedCount (uint8_t *pLedPixels, uint32_t led_count);
static int blueLedLast (uint8_t *pLedPixels, uint32_t led_count);
static void showLed (uint8_t *pLedPixels, int ledIndex);
static void showLedStrip (uint8_t *pLedPixels);
static void showLedStripHeader(void);

static Colors& colors = Colors::instance();

// Use this instead of direct #define for future support of different
// strip lengths. #define may eventually go away
static int LedCount = LED_STRIP_PIXEL_COUNT;

#define COLOR_GREEN_HSV_HIGH     120, 100, 100
#define COLOR_YELLOW_HSV_HIGH    60, 100,  100
#define COLOR_BLUE_HSV_HIGH      240, 100, 100
#define COLOR_WHITE_HSV_HIGH     0,   0,   50

//******************************************************************************
/**
 * @brief   Test color conversion RGB
 *
 */
TEST(colors, base_hsv)
{
    COLOR_HSV *pHSV = nullptr;

    pHSV = colors.getHsv (LED_COLOR_YELLOW);

    LED_COLOR get_color = colors.isHsv (pHSV);

    ASSERT_TRUE (get_color == LED_COLOR_YELLOW);

    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t rgb;

    Colors::instance().hsv2rgb(COLOR_GREEN_HSV_HIGH, &r, &g, &b);
    rgb = (r << 16) | (g << 8) | b;
    ASSERT_TRUE (Colors::instance().isRgb (rgb) == LED_COLOR_GREEN);

    Colors::instance().hsv2rgb(COLOR_YELLOW_HSV_HIGH, &r, &g, &b);
    rgb = (r << 16) | (g << 8) | b;
    ASSERT_TRUE (Colors::instance().isRgb (rgb) == LED_COLOR_YELLOW); 

    Colors::instance().hsv2rgb(COLOR_BLUE_HSV_HIGH, &r, &g, &b);
    rgb = (r << 16) | (g << 8) | b;
    ASSERT_TRUE (Colors::instance().isRgb (rgb) == LED_COLOR_BLUE);

    Colors::instance().hsv2rgb(COLOR_WHITE_HSV_HIGH, &r, &g, &b);
    rgb = (r << 16) | (g << 8) | b;
    ASSERT_TRUE (Colors::instance().isRgb (rgb) == LED_COLOR_WHITE);

}

//******************************************************************************
/**
 * @brief   Test color conversion HSV
 *
 */
TEST(colors, base_rgb)
{
    int start_color = LED_COLOR_START;
    int end_color = LED_COLOR_END;

    colors.setMode (LED_INTENSITY_LOW);

    // Loop through public LED_COLORs, convert to RGB and back again
    for (int led_color = start_color; led_color < end_color; led_color++)
    {
        uint32_t rgb_color = colors.getRgb(static_cast<LED_COLOR>(led_color));
        LED_COLOR get_color = colors.isRgb(rgb_color);

        ASSERT_TRUE ( (int)get_color == led_color);
    }

    colors.setMode (LED_INTENSITY_HIGH);

    // Loop through public LED_COLORs, convert to RGB and back again
    for (int led_color = start_color; led_color < end_color; led_color++)
    {
        uint32_t rgb_color = colors.getRgb(static_cast<LED_COLOR>(led_color));
        LED_COLOR get_color = colors.isRgb(rgb_color);
    }
}


//******************************************************************************
/**
 * @brief Animation test fixture. Needed to support parameterized runs
 *        of all the 'animation' tests. Override and then restore the
 *        LedCount global (for now).
 *
 */
class animation: public ::testing::TestWithParam<int>
{
public:
    animation(void)
    {
        savedLedCount = LedCount;
        LedCount = GetParam();
    };

    ~animation(void)
    {
        LedCount = savedLedCount;
    }

private:
    int savedLedCount;
};

//******************************************************************************
/**
 * @brief   Test Fixture for 'animation' group of tests. These tests
 *          are now run with a variety of LED counts (versus the fixed
 *          value previously used).
 */
INSTANTIATE_TEST_CASE_P(
        base,
        animation,
        ::testing::Values(
                20, 31, 32, 33, 50, 99, 100, 101, 150, 200
        ));

//******************************************************************************
/**
 * @brief Test initial display of charge at all levels from 0 to 100
 *
 * Get/Set charge levels. Validate bounds checking
 */
TEST_P(animation, chargeLevelGetSet)
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
 *        Find the "lastBlueLed", which indicates the top of the
 *        charge indicator.
 */

TEST_P(animation, chargeLevelLedsStatic)
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
    //   - Reaches but does not exceed LedCount

    int lastTopBlue = 0;
    for (int chargeLevel = 0; chargeLevel <= 100; chargeLevel++)
    {
        uint8_t led_pixels[3*LedCount] = {0};
        testAnimate.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 100);
        testAnimate.set_charge_percent(chargeLevel);
        testAnimate.refresh (led_pixels, 0, LedCount);

        int topBlue = blueLedLast(led_pixels, LedCount);

        //printf ("utest: Charge pct of %d: topBlue:%d (previous: %d)\n", chargeLevel, topBlue, lastTopBlue);

        // Always at least one; never more than max
        ASSERT_TRUE (topBlue >= 0);  // -1 if not found; this is an offset
        ASSERT_TRUE (topBlue <= LedCount);

        // Always at least previous level
        ASSERT_TRUE (topBlue >= lastTopBlue);

        // Never one more than last (for this test; real life could go backwards)
        // Also, does not apply for LED bars with more than 100 pixels. Similar
        // test for > 100 needs to be done.
        if (LedCount <= 100)
        {
            ASSERT_TRUE (topBlue - lastTopBlue <= 1);
        }

        // Save for next iteration
        lastTopBlue = topBlue;
    }

    // Did we reach the top of the LED bar? 
    ASSERT_TRUE (lastTopBlue == LedCount-1);
}

//******************************************************************************
/**
 * @brief Test animated display at a few charge levels.
 *
 *        At various charge levels, do a longer loop to
 *        test animation of LEDS.
 *
 *        Counts total number of blue pixels on the LED bar.
 *        At a fixed charge level, the number of blue LEDs
 *        should increase each cycle and revert back to three.
 *
 *        This is not as throrough as a full pixel-by-pixel check
 *        but still effective and easy to implement.
 *
 */
TEST_P(animation, chargeLevelLedsDynamic)
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

        uint8_t led_pixels[3*LedCount] = {0};

        testAnimate.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 100);
        testAnimate.set_charge_percent(chargeLevel);
        testAnimate.refresh (led_pixels, 0, LedCount);

        // Make sure we are starting with normal blue LED count (usually 3)
        int baseBlueCnt = CHARGE_BASE_LED_CNT + CHARGE_LEVEL_LED_CNT;
        ASSERT_TRUE (blueLedCount (led_pixels, LedCount) == baseBlueCnt);

        int lastBlueCnt = baseBlueCnt;
        for (int cycles = 0; cycles < 100; cycles++)
        {
            testAnimate.refresh (led_pixels, 0, LedCount);
            // Should have one more blue pixel than last time, or it's cycled
            // back to base (3 LEDs).
            int blueCnt = blueLedCount (led_pixels, LedCount);

            ASSERT_TRUE (blueCnt == lastBlueCnt+1 || blueCnt == baseBlueCnt);

            lastBlueCnt = blueCnt;
        }
    }
}

//******************************************************************************
/**
 * @brief  Check pixel fill levels when reducing from 100 to 0. Observed 
 *         bug during manual/interactive testing. 
 *
 *         KNOWN BUG:  The ChargingAnimation code knows too much 
 *                     and when charge gets back to zero, does not 
 *                     call the animation update. To fix 
 *                     this, likely need to remove the calculations 
 *                     from ChargingAnimation::refresh(). 
 * 
 */
TEST_P(animation, reduceToZero)
{
    ChargingAnimation testAnimate;
    ASSERT_FALSE (testAnimate.get_charge_simulation());

    uint8_t led_pixels[3*LedCount] = {0};

    // Since we are testing pixels, need to setup colors
    testAnimate.reset(colors.getRgb(LED_COLOR_BLUE),
        colors.getRgb(LED_COLOR_WHITE),
        colors.getRgb(LED_COLOR_BLUE), 100);

    // Just one mid-level charge for now
    testAnimate.set_charge_percent(100);
    testAnimate.refresh (led_pixels, 0, LedCount);
    uint32_t blueLeds = blueLedCount (led_pixels, LedCount);
    ASSERT_TRUE (blueLeds == 3); 

    // Validate that animation starts
    testAnimate.refresh (led_pixels, 0, LedCount);
    blueLeds = blueLedCount (led_pixels, LedCount);
    ASSERT_TRUE (blueLeds == 4); 

    // Drop the charge level. Validate animation continues 
    testAnimate.set_charge_percent(0);
    testAnimate.refresh (led_pixels, 0, LedCount);
    blueLeds = blueLedCount (led_pixels, LedCount);
    ASSERT_TRUE (blueLeds == 5);

    for (int x = blueLeds; x < LedCount; x++)
    {
        testAnimate.refresh (led_pixels, 0, LedCount);
        blueLeds = blueLedCount (led_pixels, LedCount);
        ASSERT_TRUE (blueLeds == (x+1));
    } 
}

//******************************************************************************
/**
 * @brief  Check pixel fill levels for charge level (static)
 * 
 */
TEST_P(animation, chargeLevelPixels)
{
    ChargingAnimation testAnimate;
    ASSERT_FALSE (testAnimate.get_charge_simulation());

    uint8_t led_pixels[3*LedCount] = {0};

    // Since we are testing pixels, need to setup colors
    testAnimate.reset(colors.getRgb(LED_COLOR_BLUE),
        colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 100);

    // Just one mid-level charge for now
    testAnimate.set_charge_percent(50);
    testAnimate.refresh (led_pixels, 0, LedCount);

    // first LED blue
    uint32_t colorRgb = (led_pixels[0] << 16) |
                        (led_pixels[1] << 8) |
                        (led_pixels[2] << 0);
    ASSERT_TRUE (colors.isRgb(colorRgb) == LED_COLOR_BLUE);

    // Second LED white
    colorRgb =             (led_pixels[3] << 16) |
                        (led_pixels[4] << 8) |
                        (led_pixels[5] << 0);
    ASSERT_TRUE (colors.isRgb(colorRgb) == LED_COLOR_WHITE);

    // Verify that LEDs up to charge level are non-zero. Other
    // tests validate that get_charged_led_count() is OK, so we
    // use it here.
    uint32_t chargeLeds = blueLedCount (led_pixels, LedCount);

    ASSERT_TRUE (chargeLeds > 1); 

    // Charge level BLUE shoud be nonzero 
    ASSERT_TRUE (led_pixels[3 * (chargeLeds-CHARGE_LEVEL_LED_CNT) + 2] != 0); 

}

//******************************************************************************
/**
 * @brief  Test Progress Animation object 
 *         
 */
TEST_P (animation, base)
{
    ProgressAnimation pa;  

    uint8_t led_pixels[3*LedCount] = {0};

    pa.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE));

    // Negative request and NULL array return zero 
    ASSERT_TRUE (pa.refresh (led_pixels, 0, -1) == 0); 
    ASSERT_TRUE (pa.refresh (nullptr, 0, 0) == 0); 


    // Animations have the property of retaining a previous "fill" level 
    // until animation completes. Set up a charge level of 10 pixels, then 
    // immediately request one more pixel 
    pa.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE));

    ASSERT_TRUE (pa.refresh (led_pixels, 0, 10) == 10); 
    ASSERT_TRUE (pa.refresh (led_pixels, 0, 11) == 10); 
    ASSERT_TRUE (pa.refresh (led_pixels, 0, 9) == 10); 

    // After some time, a request for a different value is "accepted" 

    for (int x = 0; x  <10; x++) 
    {
        pa.refresh(led_pixels, 0, 11);  
    }

    ASSERT_TRUE (pa.refresh (led_pixels, 0, 11) == 11); 


}

//******************************************************************************
/**
 * @brief  Test Charge Indicator animation object get/set values
 *         
 */
TEST (charge_indicator, base)
{
    ChargeIndicator ci;  

    uint8_t led_pixels[3*LedCount] = {0};

    ci.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 2);

    // Negative request and NULL array return zero 
    ASSERT_TRUE (ci.refresh (led_pixels, 0, -1) == 0); 
    ASSERT_TRUE (ci.refresh (nullptr, 0, 0) == 0); 

    // Can handle updates of less than cli size
    ci.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 2);
    ASSERT_TRUE (ci.refresh (led_pixels, 0, 1) == 1);
    int blueLeds = blueLedCount (led_pixels, LedCount);
    ASSERT_TRUE (blueLeds == 1);

    // Exactly cli_size
    ci.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 2);
    ASSERT_TRUE (ci.refresh (led_pixels, 0, 2) == 2);
    blueLeds = blueLedCount (led_pixels, LedCount);
    ASSERT_TRUE (blueLeds == 2);

    // One more than cli size (first bg pixel should appear)
    ci.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 2);
    ASSERT_TRUE (ci.refresh (led_pixels, 0, 2) == 2);
    blueLeds = blueLedCount (led_pixels, LedCount);
    ASSERT_TRUE (blueLeds == 2);

    // Changing levels during animation
    ci.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 2);
    ASSERT_TRUE (ci.refresh (led_pixels, 0, 12) == 12);
    blueLeds = blueLedCount (led_pixels, LedCount);
    ASSERT_TRUE (blueLeds == 2);
    ASSERT_TRUE (ci.refresh (led_pixels, 0, 13) == 12);  // Ask for 13, but 12 is still animating

}



//******************************************************************************
/**
 * @brief  Test static animation object 
 *         
 */
TEST (static_animation, base)
{
    StaticAnimation sa;

    uint8_t led_pixels[3*LedCount] = {0};

    sa.reset(colors.getRgb(LED_COLOR_BLUE));

    // Verify that static animations return the number of LEDs specified
    for (int x = 0; x < LedCount; x++)
    {
        ASSERT_TRUE (sa.refresh (led_pixels, 0, x) == x);
    }

    // Negative request and NULL array return zero 
    ASSERT_TRUE (sa.refresh (led_pixels, 0, -1) == 0);
    ASSERT_TRUE (sa.refresh (nullptr, 0, 0) == 0);

}


//******************************************************************************
/**
 * @brief  Count number of BLUE LEDs in an array. Helper function for
 *         testing animation.
 */
static int blueLedCount (uint8_t *pLedPixels, uint32_t led_count)
{
    int blueCount = 0;

    if (pLedPixels != NULL)
    {
        for (int x = 0; x < led_count; x++)
        {
            uint32_t colorRgb = (pLedPixels[(3 * x) + 0] << 16) |
                                (pLedPixels[(3 * x) + 1] << 8) |
                                (pLedPixels[(3 * x) + 2] << 0);

            if (colors.isRgb(colorRgb) == LED_COLOR_BLUE)
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
 * @brief  Return offset of topmost BLUE led (-1 if not found)
 */
static int blueLedLast (uint8_t *pLedPixels, uint32_t led_count)
{
    int retVal = -1;

    if (pLedPixels != NULL)
    {
        for (int x = 0; x < led_count; x++)
        {
            uint32_t colorRgb = (pLedPixels[(3 * x) + 0] << 16) |
                                (pLedPixels[(3 * x) + 1] << 8) |
                                (pLedPixels[(3 * x) + 2] << 0);

            if (colors.isRgb(colorRgb) == LED_COLOR_BLUE)
            {
                retVal = x;
            }
        }
    }

    return retVal;
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

    uint32_t rgb = r << 16 | g << 8 | b;

    LED_COLOR ledColor = colors.isRgb(rgb);

    switch (ledColor)
    {
        case LED_COLOR_WHITE:
            printf("\033[37m");
            pchar = '.';
            break; 
        case LED_COLOR_CYAN:
            printf("\033[36m");
            pchar = 'C';
            break;
        case LED_COLOR_PURPLE:  // Magenta
            printf("\033[35m");
            pchar = 'P';
            break; 
        case LED_COLOR_YELLOW:
            printf("\033[33m");
            pchar = 'Y';
            break; 
        case LED_COLOR_RED:
            printf("\033[31m");
            pchar = 'R';
            break;
        case LED_COLOR_GREEN:
            printf("\033[32m");
            pchar = 'G';
            break; 
        case LED_COLOR_BLUE:
            printf("\033[34m");
            pchar = 'B';
            break; 
        case LED_COLOR_BLACK:
            pchar = ' '; 
            break; 
        default: 
            break;
    }

    if (colors.getMode() == LED_INTENSITY_LOW)
    {
        pchar = tolower(pchar);
    }

    printf ("%c", pchar);
    printf("\033[0m");
}

//******************************************************************************
/**
 * @brief When displayed the LED bar, a header to indicate offsets.
 */
static void showLedStripHeader(void)
{

    // This is what we used to do:  Now doing it programatically to support different LED
    // strip lengths:
    //
    // printf ("LED BAR:                      1 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 3 3\n");
    // printf ("LED BAR:  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n");

    int led = LedCount;
    int div = 100;  // Handle at most 999 LEDs, should be enough!

    while (div > 0)
    {
        // Do we have digits at this level?
        if (led / div)
        {
            printf ("LED BAR:  ");
            for (int x = 0; x < LedCount; x++)
            {
                // Find single digit to (possibly) display
                int digit = div > 1 ? (x / div) % 10 : x % 10;

                // Don't print 'leading' zeros; print all other digits
                if (x < div && digit == 0 && div > 1)
                {
                    printf ("  ");
                }
                else
                {
                    printf ("%d ", digit);
                }
            }
            printf ("\n");
        }
        else
        {
            // LED Bar does not extend this high
        }

        div = div / 10;
    }
}

//******************************************************************************
/**
 * @brief Basic ASCII representation of LED bar.
 */
static void showLedStrip (uint8_t *pLedPixels)
{

    printf ("LED BAR: ");
    for (int led=0; led < LedCount; led++)
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

    int chargeBump = 5;

    while (true)
    {

        printf ("\nEnter an option:\n"
                "  <num> to animate charge level (0-100)\n"
                "  +<num> to bump charge level %d during animation\n"
                "  -<num> to cut charge level %d during animation\n"
                " 'q' or 'x' to quit\n"
                " 'l' to change LED count (currently %d\n"
                " 'd' to toggle day/night mode\n"
                " 'c' to change charge 'bump' value\n"
                " 's' to run charge simulation (long!)\n"
                "led_test> ", chargeBump, chargeBump, LedCount);

        chars_read = getline(&string, &size, stdin);

        if (chars_read < 0)
        {
            printf ("Input error");
            return;
        }
        else if (tolower(string[0]) == 'd')
        {
            colors.setMode (colors.getMode() == LED_INTENSITY_HIGH ? LED_INTENSITY_LOW:LED_INTENSITY_HIGH);
            printf ("Day/Night mode: %s\n", colors.getMode() == LED_INTENSITY_HIGH ? "Day":"Night");
        }
        else if (tolower(string[0]) == 'q' || tolower(string[0]) == 'x')
        {
            printf ("\nexiting\n");
            return;
        }
        else if (tolower(string[0]) == 's')
        {
            // Simulated charge
            ChargingAnimation testAnimate;
            testAnimate.reset(colors.getRgb(LED_COLOR_BLUE), colors.getRgb(LED_COLOR_WHITE), colors.getRgb(LED_COLOR_BLUE), 100);
            uint8_t led_pixels[3*LedCount] = {0};
            testAnimate.set_quiet (true);
            testAnimate.set_charge_simulation(true);

            showLedStripHeader();
            int numCycles = 1100;
            for (int c = 0; c < numCycles; c++)
            {
                testAnimate.refresh(led_pixels, 0, LedCount);
                showLedStrip (led_pixels);
            }

        }

        else if (tolower(string[0]) == 'l')
        {
            string[0] = ' ';
            long newLeds = strtol (string, NULL, 10);
            if (newLeds == LONG_MIN || newLeds < 0 || newLeds > 200)
            {
                printf ("Input error (must be positive value no larger than 200)");
            }
            else
            {
                LedCount = (int)newLeds;
                printf ("New LedCount: %d\n", LedCount);
            }
        }

        else if (tolower(string[0]) == 'c')
        {
            string[0] = ' ';
            long newBump = strtol (string, NULL, 10);
            if (newBump == LONG_MIN || newBump < 0 || newBump > 99)
            {
                printf ("Input error (must be positive value below 100)");
                return;
            }
            else
            {
                chargeBump = (int)newBump;
                printf ("New charge bump: %d\n", chargeBump);
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

            printf ("Animation of charge pct: %ld (bump during animate: %s)\n",
                chargePct, bumpDuringAnimate ? "yes":"no");

            ChargingAnimation testAnimate;
            testAnimate.set_quiet (true);

            uint8_t led_pixels[3*LedCount] = {0};
            testAnimate.reset(colors.getRgb(LED_COLOR_BLUE),
                colors.getRgb(LED_COLOR_WHITE),
                colors.getRgb(LED_COLOR_BLUE), 100);

            testAnimate.set_charge_percent(chargePct);

            // Try to show one full cycle of charge animation, but where possible
            // keep it on the same "page". Roughly the number of animated pixels
            // plus a few more to show the wrap-around.
            int numCycles = ((chargePct * LedCount) / 100) + 3;
            showLedStripHeader();
            for (int c = 0; c < numCycles; c++)
            {
                if (c == numCycles/2)
                {
                    // roughly haflway through. Adjust charge level
                    // if requested.
                    if (bumpDuringAnimate)
                    {
                        testAnimate.set_charge_percent(chargePct+chargeBump);
                    }
                    if (cutDuringAnimate)
                    {
                        testAnimate.set_charge_percent(MAX (chargePct-chargeBump, 0));
                    }
                }

                testAnimate.refresh(led_pixels, 0, LedCount);
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
