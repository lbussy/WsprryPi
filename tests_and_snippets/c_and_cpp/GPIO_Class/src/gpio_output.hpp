/**
 * @class GPIOOutput
 * @brief Simple GPIO output controller using libgpiod.
 *
 * This class configures a specified GPIO pin as an output. The pin can be
 * configured as active high (default) or as an active low sink. Methods are provided
 * to enable (configure) the pin, disable (release) it, and toggle its output state.
 *
 * Example:
 * @code
 * GPIOOutput gpio;
 * gpio.enable_gpio_pin(17, true); // enable pin 17 as active high output
 * gpio.toggle_gpio(true);         // set pin high (active)
 * gpio.toggle_gpio(false);        // set pin low (inactive)
 * gpio.stop_gpio_pin();        // release the pin
 * @endcode
 */

#ifndef GPIO_OUTPUT_HPP
#define GPIO_OUTPUT_HPP

#include <memory>
#include <string>
#include <gpiod.hpp>

class GPIOOutput
{
public:
    /**
     * @brief Default constructor.
     *
     * Constructs an inactive GPIOOutput object.
     */
    GPIOOutput();

    /**
     * @brief Destructor.
     *
     * Releases the GPIO pin if it has been enabled.
     */
    ~GPIOOutput();

    /**
     * @brief Configures and enables a GPIO pin for output.
     *
     * Opens the default GPIO chip (/dev/gpiochip0), obtains the specified pin,
     * and requests it as an output. The pin can be configured as active high or
     * active low (sink).
     *
     * @param pin The GPIO pin number (BCM numbering).
     * @param active_high True for active-high operation (default), false for sink.
     * @return True if the pin was successfully configured; false otherwise.
     */
    bool enable_gpio_pin(int pin, bool active_high = true);

    /**
     * @brief Disables the GPIO pin.
     *
     * Releases the line resource if it is currently in use.
     */
    void stop_gpio_pin();

    /**
     * @brief Sets the GPIO output state.
     *
     * For an active-high pin, passing true sets the pin high; for an active-low
     * (sink) pin, the logic is inverted (i.e. passing true sets the pin low).
     *
     * @param state Desired logical state (true for active, false for inactive).
     * @return True if the state was successfully set; false otherwise.
     */
    bool toggle_gpio(bool state);

private:
    int pin_;
    bool active_high_;
    bool enabled_;

    // Using unique_ptr to manage the libgpiod chip and line objects.
    std::unique_ptr<gpiod::chip> chip_;
    std::unique_ptr<gpiod::line> line_;

    // Helper to compute the physical state to write based on active configuration.
    int compute_physical_state(bool logical_state) const;
};

#endif // GPIO_OUTPUT_HPP
