// tests.cpp

#include <stdlib.h>
#include <limits.h>

#include <gtest/gtest.h>
#include "Led.h"
#include "ChargingAnimation.h"
#include "Utils/Colors.h"

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

TEST(animation, chargeLevelLedsStatic)
{
    ChargingAnimation testAnimate;

    ASSERT_FALSE (testAnimate.get_charge_simulation());

    uint8_t led_pixels[3*LED_STRIP_PIXEL_COUNT] = {0};

    // Remain agnostic towards algorithm, but validate a few
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

// Prints a single LED status in ASCII format from
// pasded Led Pixel array at offset.
void showLED (uint8_t *pLedPixels, int ledIndex)
{
    uint8_t r = *(pLedPixels + (ledIndex*3) + 0);
    uint8_t g = *(pLedPixels + (ledIndex*3) + 1);
    uint8_t b = *(pLedPixels + (ledIndex*3) + 2);
    char pchar = '*';
    uint32_t rgb = b + r + g;

    // Left pixel bar (in black)
    printf ("|");

    if (rgb == 0xFF + 0xFF + 0xFF)
    {
        printf("\033[37m");
        pchar = 'W';
    }
    else if (rgb == 0xFF + 0xFF)
    {
        if (r == 0)
        {
            printf("\033[36m");
            pchar = 'C';
        }
        else if (g == 0)
        {
            printf("\033[35m");
            pchar = 'M';
        }
        else if (b == 0)
        {
            printf("\033[33m");
            pchar = 'Y';
        }
    }
    else if (rgb == 0xFF)
    {
        if (r == 0xFF)
        {
            printf("\033[31m");
            pchar = 'R';
        }
        else if (g == 0xFF)
        {
            printf("\033[32m");
            pchar = 'G';
        }
        else if (b == 0xFF)
        {
            printf("\033[34m");
            pchar = 'B';
        }
    }
    else
    {
        pchar = '.';
    }

    printf ("%c", pchar);
    printf("\033[0m");
}

void showBarHeader(void)
{
    printf ("LED BAR:  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n");
}

void showBar (uint8_t *pLedPixels)
{

    printf ("LED BAR: ");
    for (int led=0; led < LED_STRIP_PIXEL_COUNT; led++)
    {
        showLED(pLedPixels, led);
    }
    printf ("|\n");
}


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

        showBarHeader();
        for (int c = 0; c < 50; c++)
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
