#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "WS2812.pio.h"
#include "drivers/leds/leds.h"
#include "drivers/lis3dh/lis3dh.h"
#include "drivers/microphone/microphone.h"
#include "board.h"
#include <stdio.h>

// Mode switching
enum AppMode {
    MODE_LEDS = 0,        // Lab 2
    MODE_ACCELEROMETER,   // Lab 3
    MODE_MICROPHONE,      // Lab 4
    MODE_COUNT
};
 
static AppMode current_mode = MODE_MICROPHONE;
 
static void setup_mode_button(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_disable_pulls(BUTTON_PIN); // external 100k pull-down already present
}
 
// Returns true once per button press (rising edge, active-high).
static bool button_pressed(void)
{
    static bool last_state = false; // idle low (pulled down by R6)
 
    bool pressed = gpio_get(BUTTON_PIN);
    bool triggered = false;
 
    if (pressed && !last_state) {
        triggered = true;
        sleep_ms(200); 
    }
 
    last_state = pressed;
    return triggered;
}
 
// Lab 2: LED demo
static void led_task(LEDDriver &leds)
{
    static uint16_t offset_hue = 0;
 
    for (int i = 0; i < NUM_LEDS; i++) {
        uint16_t hue = (offset_hue + (360 / NUM_LEDS) * i) % 360;
        leds.set_hsv(i, {hue, 255, 150});
    }
    leds.show();
    offset_hue = (offset_hue + 5) % 360;
    sleep_ms(50);
}
 
// Lab 3: Accelerometer spirit level
static float clampf(float v) {
    if (v < -1.0f) return -1.0f;
    if (v >  1.0f) return  1.0f;
    return v;
}
 
// X tilt: negative = tilted left, positive = tilted right.
// Light the LEFT column when tilting left, RIGHT column when tilting right.
// The higher the tilt the further up the column the LED moves.
static void show_x_tilt(LEDDriver &leds, float x) {
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
static void show_y_tilt(LEDDriver &leds, float y) {
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
 
static void accelerometer_task(LEDDriver &leds, LIS3DH &accel)
{
    AccelRaw raw = accel.read_raw();
    AccelG   g   = accel.to_g(raw);
 
    float x = clampf(g.x);
    float y = clampf(g.y);
 
    // tilt = 0 when perfectly flat (x=0, y=0), grows as board tilts
    float tilt = x * x + y * y;

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
 
// Lab 4: Microphone / pitch detection
static uint16_t adc_buffer[MIC_SAMPLE_COUNT];          // raw ADC samples
static int16_t  time_domain[MIC_SAMPLE_COUNT];         // Q15 time domain signal
static int16_t  freq_domain[MIC_FFT_OUTPUT_SIZE];      // FFT output (interleaved re/im)
static q15_t    mag_squared[MIC_MAG_OUTPUT_SIZE];      // |FFT|^2
 
// Logarithmically spaced FFT bin boundaries (13 edges -> NUM_LEDS bands)
// Based on ceil(logspace(log10(5), log10(512), 13)), last edge capped at 513
// to stay within the usable energy range.
static const int BIN_EDGES[NUM_LEDS + 1] = {
    6, 8, 11, 16, 24, 35, 51, 75, 110, 161, 237, 349, 513
};
 
// Threshold filters out noise floor; scale sets sensitivity.
// These values will need tuning based on your environment.
static constexpr int32_t ENERGY_THRESHOLD = 250;
static constexpr int32_t ENERGY_SCALE     = 1000;
 
static void microphone_task(LEDDriver &leds)
{
    // Read MIC_SAMPLE_COUNT samples from the microphone
    microphone_read(adc_buffer, MIC_SAMPLE_COUNT);
 
    // Remove DC bias and convert to Q15 time-domain signal
    microphone_process(adc_buffer, time_domain, MIC_SAMPLE_COUNT);
 
    // Apply the Hanning window
    microphone_apply_window(time_domain, MIC_SAMPLE_COUNT);
 
    // Compute the FFT
    microphone_fft(time_domain, freq_domain);
 
    // Magnitude squared (energy spectral density)
    microphone_magnitude_squared(freq_domain, mag_squared);
 
    // Display result on the WS2812 LED strip using logarithmically spaced bins
    leds.clear();
    for (int led = 0; led < NUM_LEDS; led++) {
        int bin_start = BIN_EDGES[led];
        int bin_end   = BIN_EDGES[led + 1]; // exclusive
 
        // Sum energy across all bins in this LED's frequency band.
        int32_t energy = 0;
        for (int b = bin_start; b < bin_end; b++) {
            energy += mag_squared[b];
        }
 
        if (energy < ENERGY_THRESHOLD) {
            leds.set(led, Colours::OFF);
        } else {
            // Clamp to 0-255
            int32_t brightness = ((energy - ENERGY_THRESHOLD) * 255) / ENERGY_SCALE;
            if (brightness > 255) brightness = 255;
 
            // Green at low energy, red at high energy
            Colour c;
            c.red   = (uint8_t)(brightness);
            c.green = (uint8_t)(255 - brightness);
            c.blue  = 0;
            leds.set(led, c);
        }
    }
    leds.show();
}
 
// Main
int main()
{
    stdio_init_all();
 
    // Give USB serial time to connect before printing
    sleep_ms(2000);
    printf("CC3501 Labs - LEDs / Accelerometer / Microphone\n");
 
    setup_mode_button();
 
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, LED_PIN, 800000, false);
    LEDDriver leds(pio0, 0);
    leds.clear();
    leds.show();
 
    LIS3DH accel;
    bool accel_ok = accel.init();
    if (!accel_ok) {
        printf("Accelerometer init FAILED\n");
    } else {
        printf("Accelerometer init OK\n");
    }
 
    microphone_init();
    printf("Microphone init OK\n");
 
    printf("Press the mode button to cycle: LEDs -> Accelerometer -> Microphone\n");
 
    while (true) {
        if (button_pressed()) {
            current_mode = (AppMode)((current_mode + 1) % MODE_COUNT);
            leds.clear();
            leds.show();
 
            switch (current_mode) {
                case MODE_LEDS:
                    printf("Mode: LEDs\n");
                    break;
                case MODE_ACCELEROMETER:
                    printf("Mode: Accelerometer\n");
                    break;
                case MODE_MICROPHONE:
                    printf("Mode: Microphone\n");
                    break;
                default:
                    break;
            }
        }
 
        switch (current_mode) {
            case MODE_LEDS:
                led_task(leds);
                break;
            case MODE_ACCELEROMETER:
                if (accel_ok) {
                    accelerometer_task(leds, accel);
                } else {
                    // Indicate accelerometer failure with all-red LEDs
                    for (int i = 0; i < NUM_LEDS; i++) {
                        leds.set(i, Colours::RED);
                    }
                    leds.show();
                    sleep_ms(500);
                }
                break;
            case MODE_MICROPHONE:
                microphone_task(leds);
                break;
            default:
                break;
        }
    }
 
    return 0;
}