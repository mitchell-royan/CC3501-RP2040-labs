#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "WS2812.pio.h"
#include "drivers/leds/leds.h"
#include "drivers/lis3dh/lis3dh.h"
#include "board.h"
#include <stdio.h>

int main() {
    stdio_init_all();
    sleep_ms(10000);  // give USB serial time to connect

    LIS3DH accel;
    if (!accel.init()) {
        printf("Accelerometer init FAILED\n");
        while (true) {}
    }
    printf("Accelerometer init OK\n");
    while (true) {}
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