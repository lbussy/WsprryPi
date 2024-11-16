# Pulse-Width Modulation on the Raspberry Pi

To create a C++ program that can automatically detect and work across Raspberry Pi models 1 through 5, we can use runtime parameters to set PWM on the appropriate GPIO pin with user-defined frequency and duty cycle. The program will check for hardware support, use pigpio for PWM, and fall back to software PWM if needed.

## Code Sample

Here's the code that supports runtime configuration, allowing it to work on any Raspberry Pi model, Zero and 1 to 5:

``` cpp
#include <iostream>
#include <pigpio.h>

// Function to start PWM on a given pin with specified frequency and duty cycle
bool startPWM(int pin, int frequency, int dutyCyclePercentage) {
    // Initialize pigpio library
    if (gpioInitialise() < 0) {
        std::cerr << "pigpio initialization failed." << std::endl;
        return false;
    }

    // Convert duty cycle percentage to a value between 0 and 255
    int dutyCycle = (dutyCyclePercentage * 255) / 100;

    // Set PWM frequency on the specified pin
    if (gpioSetPWMfrequency(pin, frequency) < 0) {
        std::cerr << "Failed to set PWM frequency on GPIO " << pin << std::endl;
        gpioTerminate();
        return false;
    }

    // Start PWM with the calculated duty cycle
    if (gpioPWM(pin, dutyCycle) < 0) {
        std::cerr << "Failed to start PWM on GPIO " << pin << std::endl;
        gpioTerminate();
        return false;
    }

    std::cout << "PWM started on GPIO " << pin
              << " at frequency: " << frequency << " Hz"
              << " with duty cycle: " << dutyCyclePercentage << "%" << std::endl;
    return true;
}

void stopPWM(int pin) {
    gpioPWM(pin, 0);        // Set duty cycle to 0 to stop
    gpioTerminate();         // Clean up resources
}

int main() {
    int pin;
    int frequency;
    int dutyCyclePercentage;

    // Prompt the user for GPIO pin, frequency, and duty cycle
    std::cout << "Enter GPIO pin number for PWM (e.g., 18): ";
    std::cin >> pin;
    std::cout << "Enter PWM frequency in Hz (e.g., 1000): ";
    std::cin >> frequency;
    std::cout << "Enter duty cycle percentage (0-100): ";
    std::cin >> dutyCyclePercentage;

    // Start PWM based on the user input
    if (startPWM(pin, frequency, dutyCyclePercentage)) {
        std::cout << "Press Enter to stop PWM...";
        std::cin.ignore();
        std::cin.get();
        stopPWM(pin);
    } else {
        std::cerr << "Unable to initialize PWM on specified GPIO pin." << std::endl;
    }

    return 0;
}
```

### Explanation of the Code

1. Dynamic Pin Selection: The user can choose the GPIO pin for PWM output, which is especially useful because each Pi model has specific hardware PWM pins.
2. Frequency and Duty Cycle: The frequency and duty cycle are also configurable at runtime, making this program flexible for various applications.
3. Error Checking: The program checks for errors when initializing PWM, setting the frequency, and starting PWM. If an error occurs, it cleans up and exits.
4. Cross-Model Support: By allowing the user to define the GPIO pin at runtime, this program supports the Pi 1 through Pi 5. GPIO 18 and GPIO 19 support hardware PWM across multiple Pi models, making them preferred choices if available.

### Usage Instructions

 1. Compile and Run:

    ``` bash
    g++ -o pwm_example pwm_example.cpp -lpigpio -lrt
    sudo ./pwm_example
    ```

2. Input Parameters:
    * GPIO Pin: Choose a pin that supports PWM on your Pi model (e.g., GPIO 18 or 19).
    * Frequency: Enter the desired frequency in Hz (e.g., 1000 for 1 kHz).
    * Duty Cycle: Enter the duty cycle as a percentage (0–100).

3. Stopping PWM: Press Enter to stop the PWM signal, which triggers cleanup.

### Notes

* Recommended Pins: For best results, use GPIO 18 or GPIO 19 on all Pi models that support hardware PWM.
* Frequency Limitations: High frequencies are best achieved on hardware PWM-supported pins.

## Frequency Range

The achievable PWM frequency ranges in Hz on Raspberry Pi models depend on whether the code uses hardware PWM (available on specific GPIO pins) or software PWM (available on any GPIO pin, but with limitations in accuracy and frequency stability). Here are the frequency ranges for each Raspberry Pi model when using pigpio:

1. Raspberry Pi 1 and Zero
    * Hardware PWM (GPIO 18 and GPIO 19):
        * Frequency range: Up to about 19.2 MHz (the base oscillator frequency for the Pi 1 and Zero).
        * Practical range for accurate PWM: Up to 1 MHz for stable signals.
    * Software PWM (Any GPIO pin):
        * Frequency range: Up to around 1 kHz.
        * Practical range for accurate PWM: 1–500 Hz. Above this, timing may become unreliable due to CPU and OS scheduling limitations.

2. Raspberry Pi 2
    * Hardware PWM (GPIO 18 and GPIO 19):
        * Frequency range: Up to about 19.2 MHz.
        * Practical range for stable signals: Up to 1 MHz.
    * Software PWM (Any GPIO pin):
        * Frequency range: Up to around 1 kHz.
        * Practical range: 1–500 Hz. As with the Pi 1, software PWM may become unreliable at higher frequencies.

3. Raspberry Pi 3

    * Hardware PWM (GPIO 18 and GPIO 19):
        * Frequency range: Up to about 19.2 MHz.
        * Practical range: Up to 1 MHz for reliable and stable PWM signals.

    * Software PWM (Any GPIO pin):
        * Frequency range: Up to around 1 kHz.
        * Practical range: 1–500 Hz, though performance can improve slightly over Pi 1 and 2.

4. Raspberry Pi 4

    * Hardware PWM (GPIO 18 and GPIO 19):
        * Frequency range: Up to about 50 MHz (the base oscillator frequency for Pi 4 is 54 MHz).
        * Practical stable range: Up to 5 MHz for accurate PWM, though lower frequencies (up to 1 MHz) are generally more stable.

    * Software PWM (Any GPIO pin):
        * Frequency range: Up to around 1 kHz.
        * Practical range: 1–500 Hz, with better performance than earlier models due to improved CPU and multi-core support.

5. Raspberry Pi 5

    * Hardware PWM (GPIO 18 and GPIO 19):
        * Frequency range: Up to about 100 MHz (due to the RP1 I/O controller and improved clock source).
        * Practical stable range: Up to 10 MHz for reliable PWM signals, though most applications use frequencies below this for stability.

    * Software PWM (Any GPIO pin):
        * Frequency range: Up to around 1 kHz.
        * Practical range: 1–500 Hz, improved by the higher performance of the RP1 controller and multi-core CPU.

### Summary Table

| **Model**         | **Hardware PWM**      | **Software PWM**  |
| :---------------- | :-------------------- | ----------------: |
| Raspberry Pi Zero | Up to 1 MHz (stable)  | 1-500 Hz          |
| Raspberry Pi 1	| Up to 1 MHz (stable)	| 1–500 Hz          |
| Raspberry Pi 2	| Up to 1 MHz (stable)	| 1–500 Hz          |
| Raspberry Pi 3	| Up to 1 MHz (stable)	| 1–500 Hz          |
| Raspberry Pi 4	| Up to 5 MHz (stable)  | 1–500 Hz          |
| Raspberry Pi 5	| Up to 10 MHz (stable)	| 1–500 Hz          |

## Practical Recommendations

* Use Hardware PWM: For higher frequencies and more stable signals, use GPIO 18 or GPIO 19, which support hardware PWM.
* Low-Frequency Applications: For applications that don’t require high-frequency accuracy (e.g., LED dimming), software PWM on any GPIO pin will generally work up to around 1 kHz.
* High-Frequency Applications: For frequencies above 500 Hz, always use hardware PWM if possible.
