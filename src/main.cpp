#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "WS2812.pio.h"
#include "drivers/logging/logging.h"
#include "drivers/leds.h"

#define LED_PIN  14
#define NUM_LEDS 12


/// Demo 1 — Solid colour: turn all LEDs the same colour, then clear them.
static void demo_solid_colour(LEDDriver &leds)
{
    log(LogLevel::INFORMATION, "Demo 1: solid colour (red)");

    leds.set_all(RGBColour(180, 0, 0));
    leds.commit();
    sleep_ms(1000);

    leds.clear();
    leds.commit();
    sleep_ms(500);
}

/// Demo 2 — Independent LEDs: change two LEDs in one commit without touching
///           the others.
static void demo_independent_leds(LEDDriver &leds)
{
    log(LogLevel::INFORMATION, "Demo 2: set LED 2 blue, LED 5 green, others off");

    // Stage two individual changes — the driver remembers all other LEDs.
    leds.set_led(1, RGBColour(0, 0, 200)); // LED 2 blue (0-indexed)
    leds.set_led(4, RGBColour(0, 200, 0)); // LED 5 green (0-indexed)

    // Query pending-change flag before committing
    if (leds.has_pending_changes()) {
        log(LogLevel::INFORMATION, "  Pending changes detected — committing...");
    }

    leds.commit();
    sleep_ms(1000);

    leds.clear();
    leds.commit();
    sleep_ms(500);
}

/// Demo 3 — Batch update: set a run of LEDs in a single call.
static void demo_batch_update(LEDDriver &leds)
{
    log(LogLevel::INFORMATION, "Demo 3: batch update — LEDs 5-8 white");

    RGBColour batch[4] = {
        RGBColour(200, 200, 200),
        RGBColour(200, 200, 200),
        RGBColour(200, 200, 200),
        RGBColour(200, 200, 200),
    };
    leds.set_leds(4, batch, 4);
    leds.commit();
    sleep_ms(1000);

    leds.clear();
    leds.commit();
    sleep_ms(500);
}

/// Demo 4 — Query current status of the LEDs.
static void demo_query(LEDDriver &leds)
{
    log(LogLevel::INFORMATION, "Demo 4: querying LED state");

    leds.set_led(0, RGBColour(100, 50, 25));
    leds.commit();

    // Read back the value from the driver's internal buffer.
    RGBColour c = leds.get_led(0);
    printf("  LED 0 staged colour: R=%u G=%u B=%u\n", c.red, c.green, c.blue);

    // Pending-change flag should be false right after commit.
    printf("  has_pending_changes() = %s\n",
           leds.has_pending_changes() ? "true" : "false");

    // Stage another change and check again.
    leds.set_led(0, RGBColour(0, 100, 0));
    printf("  After set_led(0, green): has_pending_changes() = %s\n",
           leds.has_pending_changes() ? "true" : "false");

    leds.commit();
    sleep_ms(1000);

    leds.clear();
    leds.commit();
    sleep_ms(500);
}

/// Demo 5 — HSV rainbow: sweep hue around the circle across all LEDs.
static void demo_hsv_rainbow(LEDDriver &leds)
{
    log(LogLevel::INFORMATION, "Demo 5: HSV rainbow sweep");

    const uint  num_leds = leds.get_num_leds();
    const float hue_step = 360.0f / (float)num_leds;

    // Animate for 3 full rotations
    for (int frame = 0; frame < 3 * 360; ++frame) {
        float base_hue = (float)frame;
        for (uint i = 0; i < num_leds; ++i) {
            float hue = base_hue + hue_step * (float)i;
            leds.set_led(i, HSVColour(hue, 1.0f, 0.4f));
        }
        leds.commit();
        sleep_ms(8);
    }

    leds.clear();
    leds.commit();
    sleep_ms(500);
}

/// Demo 6 — HSV "breathe": pulse a single colour in and out using HSV value.
static void demo_hsv_breathe(LEDDriver &leds)
{
    log(LogLevel::INFORMATION, "Demo 6: HSV breathe (cyan)");

    const float hue = 180.0f; // cyan
    const int   steps = 60;

    for (int cycle = 0; cycle < 3; ++cycle) {
        // Fade in
        for (int s = 0; s <= steps; ++s) {
            float v = (float)s / (float)steps;
            leds.set_all(HSVColour(hue, 1.0f, v * 0.6f));
            leds.commit();
            sleep_ms(16);
        }
        // Fade out
        for (int s = steps; s >= 0; --s) {
            float v = (float)s / (float)steps;
            leds.set_all(HSVColour(hue, 1.0f, v * 0.6f));
            leds.commit();
            sleep_ms(16);
        }
    }

    leds.clear();
    leds.commit();
    sleep_ms(500);
}


int main()
{
    stdio_init_all();

    // Initialise the PIO program once, then hand the offset to the driver.
    uint pio_program_offset = pio_add_program(pio0, &ws2812_program);

    // Construct the LED driver — the driver calls ws2812_program_init()
    // internally, so there is no need to call it manually.
    LEDDriver leds(pio0, 0, pio_program_offset, LED_PIN, NUM_LEDS);

    log(LogLevel::INFORMATION, "LED driver initialised");

    for (;;) {
        demo_solid_colour(leds);
        demo_independent_leds(leds);
        demo_batch_update(leds);
        demo_query(leds);
        demo_hsv_rainbow(leds);
        demo_hsv_breathe(leds);
    }

    return 0;
}