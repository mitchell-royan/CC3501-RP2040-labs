// WS2812 LED chain driver — buffered, object-oriented interface.

#include "leds.h"

#include <stdlib.h>    // malloc / free
#include <string.h>    // memcpy / memset

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "WS2812.pio.h"


RGBColour hsv_to_rgb(HSVColour hsv)
{
    // Clamp inputs
    float h = hsv.hue;
    float s = hsv.saturation;
    float v = hsv.value;

    if (s <= 0.0f) {
        // Achromatic (grey)
        uint8_t grey = (uint8_t)(v * 255.0f);
        return RGBColour(grey, grey, grey);
    }

    // Wrap hue into [0, 360)
    while (h >= 360.0f) h -= 360.0f;
    while (h <    0.0f) h += 360.0f;

    float sector = h / 60.0f;
    int   i      = (int)sector;
    float f      = sector - (float)i; // fractional part of sector

    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    float r, g, b;
    switch (i) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break; // case 5
    }

    return RGBColour(
        (uint8_t)(r * 255.0f),
        (uint8_t)(g * 255.0f),
        (uint8_t)(b * 255.0f)
    );
}

// LEDDriver — constructor / destructor

LEDDriver::LEDDriver(PIO pio, uint sm, uint program_offset, uint pin, uint num_leds)
    : _pio(pio), _sm(sm), _pin(pin), _num_leds(num_leds), _dirty(false)
{
    // Allocate and zero-initialise both buffers
    _staged    = new RGBColour[num_leds]();
    _committed = new RGBColour[num_leds]();

    // Initialise the PIO state machine for the WS2812 protocol
    ws2812_program_init(pio, sm, program_offset, pin, 800000, false);
}

LEDDriver::~LEDDriver()
{
    delete[] _staged;
    delete[] _committed;
}

// Private helpers

uint32_t LEDDriver::pack_grb(RGBColour c)
{
    // The WS2812 protocol uses GRB order.
    // The PIO program shifts out the top 24 bits MSB-first.
    return ((uint32_t)c.red << 24)
         | ((uint32_t)c.green   << 16)
         | ((uint32_t)c.blue  <<  8);
}

// Single-LED control

void LEDDriver::set_led(uint index, RGBColour colour)
{
    if (index >= _num_leds) return;

    if (_staged[index] != colour) {
        _staged[index] = colour;
        _dirty = true;
    }
}

void LEDDriver::set_led(uint index, HSVColour colour)
{
    set_led(index, hsv_to_rgb(colour));
}

// Multi-LED control

void LEDDriver::set_leds(uint start_index, const RGBColour *colours, uint count)
{
    for (uint i = 0; i < count; ++i) {
        set_led(start_index + i, colours[i]);
    }
}

void LEDDriver::set_all(RGBColour colour)
{
    for (uint i = 0; i < _num_leds; ++i) {
        set_led(i, colour);
    }
}

void LEDDriver::set_all(HSVColour colour)
{
    set_all(hsv_to_rgb(colour));
}

// Convenience

void LEDDriver::clear()
{
    set_all(RGBColour(0, 0, 0));
}

// Commit

void LEDDriver::commit()
{
    for (uint i = 0; i < _num_leds; ++i) {
        pio_sm_put_blocking(_pio, _sm, pack_grb(_staged[i]));
    }

    // Mirror staged → committed and clear dirty flag
    for (uint i = 0; i < _num_leds; ++i) {
        _committed[i] = _staged[i];
    }
    _dirty = false;
}

// Query

RGBColour LEDDriver::get_led(uint index) const
{
    if (index >= _num_leds) return RGBColour();
    return _staged[index];
}

uint LEDDriver::get_num_leds() const
{
    return _num_leds;
}

bool LEDDriver::has_pending_changes() const
{
    return _dirty;
}