#pragma once

#include <stdint.h>
#include "hardware/pio.h"

/// Represents a colour in RGB format.
struct RGBColour {
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    RGBColour(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0)
        : red(r), green(g), blue(b) {}

    bool operator==(const RGBColour &other) const {
        return red == other.red && green == other.green && blue == other.blue;
    }

    bool operator!=(const RGBColour &other) const {
        return !(*this == other);
    }
};

/// Represents a colour in HSV format.
/// Hue: 0.0–360.0 degrees, Saturation: 0.0–1.0, Value: 0.0–1.0
struct HSVColour {
    float hue;        // 0.0 – 360.0
    float saturation; // 0.0 – 1.0
    float value;      // 0.0 – 1.0

    HSVColour(float h = 0.0f, float s = 0.0f, float v = 0.0f)
        : hue(h), saturation(s), value(v) {}
};

/// Converts an HSV colour to an RGB colour.
RGBColour hsv_to_rgb(HSVColour hsv);

/// A driver for a chain of WS2812 RGB LEDs.
class LEDDriver {
public:
    /// @param pio            The PIO instance to use (e.g. pio0).
    /// @param sm             The state machine index within that PIO.
    /// @param program_offset The offset returned by pio_add_program().
    /// @param pin            The GPIO pin connected to the LED data line.
    /// @param num_leds       The number of LEDs in the chain.
    LEDDriver(PIO pio, uint sm, uint program_offset, uint pin, uint num_leds = 12);

    /// Destructor — frees the heap-allocated colour buffer.
    ~LEDDriver();

    // Single-LED control

    /// Stage a colour change for a single LED (0-indexed).
    /// Call commit() to push the change to the hardware.
    /// Does nothing if index is out of range.
    void set_led(uint index, RGBColour colour);

    /// Convenience overload accepting an HSV colour.
    void set_led(uint index, HSVColour colour);

    // Multi-LED control

    /// Stage colour changes for a contiguous range of LEDs.
    ///
    /// @param start_index First LED to update (0-indexed, inclusive).
    /// @param colours     Pointer to an array of RGBColour values.
    /// @param count       Number of LEDs to update.
    ///
    /// Out-of-range LEDs are silently skipped.
    void set_leds(uint start_index, const RGBColour *colours, uint count);

    /// Stage the same colour for every LED in the chain.
    void set_all(RGBColour colour);

    /// Convenience overload accepting an HSV colour.
    void set_all(HSVColour colour);

    // Convenience

    /// Stage all LEDs off (black).
    void clear();

    // Commit

    /// Send the currently staged LED values to the hardware.
    /// After this call, has_pending_changes() returns false.
    void commit();

    // Query

    /// Return the currently staged colour for the LED at @p index.
    /// Returns black {0,0,0} if index is out of range.
    RGBColour get_led(uint index) const;

    /// Return the total number of LEDs this driver manages.
    uint get_num_leds() const;

    /// Returns true if any LED has been staged but not yet committed.
    bool has_pending_changes() const;

private:
    PIO   _pio;
    uint  _sm;
    uint  _pin;
    uint  _num_leds;

    RGBColour *_staged;   ///< Staged (not yet committed) values
    RGBColour *_committed;///< Last values written to the hardware

    bool _dirty; ///< True when staged != committed

    /// Pack an RGBColour into the 32-bit word the WS2812 PIO program expects.
    /// The PIO program expects GRB order in the top 24 bits (MSB-first shift).
    static uint32_t pack_grb(RGBColour c);
};