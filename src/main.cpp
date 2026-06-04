#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "WS2812.pio.h"
#include "drivers/leds/leds.h"
#include "drivers/lis3dh/lis3dh.h"
#include "board.h"
#include <stdio.h>

static float clampf(float v) {
    if (v < -1.0f) return -1.0f;
    if (v >  1.0f) return  1.0f;
    return v;
}
 
// X tilt: negative = tilted left, positive = tilted right.
// Light the LEFT column when tilting left, RIGHT column when tilting right.
// The higher the tilt the further up the column the LED moves.
static void show_x_tilt(LEDDriver& leds, float x) {
    float mag = x < 0 ? -x : x;
    if (mag < 0.1f) return;
 
    // Map 0..1 tilt to row index 0..3 (0 = bottom, 3 = top)
    int row = (int)(mag * 3.5f);
    if (row > 3) row = 3;
 
    if (x < 0) {
        // Tilted left — light left column (0=bottom, 3=top)
        leds.set(row, Colours::RED);
    } else {
        // Tilted right — light right column (11=bottom, 8=top)
        leds.set(11 - row, Colours::RED);
    }
}
 
// Y tilt: positive = tilted forward (top goes down), negative = tilted back.
// Light the TOP row when tilting forward, bottom corners when tilting back.
static void show_y_tilt(LEDDriver& leds, float y) {
    float mag = y < 0 ? -y : y;
    if (mag < 0.1f) return;
 
    if (y > 0) {
        // Tilted forward — light a position across the top row (4-7)
        int pos = (int)(mag * 3.5f);
        if (pos > 3) pos = 3;
        leds.set(4 + pos, Colours::BLUE);
    } else {
        // Tilted back — light both bottom corners
        leds.set(0, Colours::BLUE);
        leds.set(11, Colours::BLUE);
    }
}
 
void spirit_level(LEDDriver& leds, LIS3DH& accel) {
    while (true) {
        AccelRaw raw = accel.read_raw();
        AccelG   g   = accel.to_g(raw);
 
        float x = clampf(g.x);
        float y = clampf(g.y);
 
        // tilt = 0 when perfectly flat (x=0, y=0), grows as board tilts
        float tilt = x * x + y * y;
 
        printf("X: %6.3fg  Y: %6.3fg  Z: %6.3fg\n", g.x, g.y, g.z);
 
        leds.clear();
 
        if (tilt < 0.02f) {
            // Perfectly level — all green
            for (int i = 0; i < NUM_LEDS; i++) {
                leds.set(i, Colours::GREEN);
            }
        } else {
            // Tilted — show red for X axis, blue for Y axis
            show_x_tilt(leds, x);
            show_y_tilt(leds, y);
        }
 
        leds.show();
        sleep_ms(100);
    }
}
 
int main() {
    stdio_init_all();
    sleep_ms(500);
 
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, LED_PIN, 800000, false);
    LEDDriver leds(pio0, 0);
 
    LIS3DH accel;
    if (!accel.init()) {
        printf("Accelerometer init FAILED\n");
        for (int i = 0; i < NUM_LEDS; i++) {
            leds.set(i, Colours::RED);
        }
        leds.show();
        while (true) {}
    }
    printf("Accelerometer init OK — spirit level running\n\n");
 
    spirit_level(leds, accel);
}

//     // Set up the WS2812 PIO program and create the driver
//     uint offset = pio_add_program(pio0, &ws2812_program);
//     ws2812_program_init(pio0, 0, offset, LED_PIN, 800000, false);
//     LEDDriver leds(pio0, 0);

//     sleep_ms(500);  // Wait for everything to settle after initialization

//     // Example 1: Single LED 
//     leds.set(0, Colours::RED);
//     leds.show();
//     sleep_ms(1000);

//     // Example 2: Stage multiple changes, then send at once
//     leds.set(0, Colours::OFF);
//     leds.set(1, Colours::GREEN);
//     leds.set(2, Colours::BLUE);
//     leds.set(3, Colours::ORANGE);
//     leds.show();
//     sleep_ms(1000);

//     // Example 3: Set multiple LEDs in a single call
//     LEDUpdate updates[] = {
//         {4, Colours::PURPLE},
//         {5, Colours::WHITE},
//         {6, {100, 200, 50}},  // custom colour
//     };
//     leds.set_multiple(updates, 3);
//     leds.show();
//     sleep_ms(1000);

//     // Example 4: HSV colour
//     leds.clear();
//     leds.set_hsv(0, {120, 255, 180});  // green via HSV
//     leds.show();
//     sleep_ms(1000);

//     // Example 5: Query an LED's colour
//     Colour c = leds.get(0);
//     (void)c;  // c.red, c.green, c.blue are available

//     // Example 6: Only send if something changed
//     leds.set(0, Colours::WHITE);
//     if (leds.is_dirty()) {
//         leds.show();
//     }
//     sleep_ms(1000);

//     // Example 7: Spinning rainbow animation using HSV  
//     uint16_t offset_hue = 0;
//     while (true) {
//         for (int i = 0; i < NUM_LEDS; i++) {
//             uint16_t hue = (offset_hue + (360 / NUM_LEDS) * i) % 360;
//             leds.set_hsv(i, {hue, 255, 150});
//         }
//         leds.show();
//         offset_hue = (offset_hue + 5) % 360;
//         sleep_ms(50);
//     }
// }