# Transmitting a Tone on SSB

This demo code will work across all Raspberry Pi models (Zero, 1, 2, 3, 4, and 5) and allows for a user-selectable carrier frequency, sideband type (USB or LSB), and tone frequency (between 1400 and 1600 Hz).

This program:

1. Detects the Raspberry Pi model at runtime.
2. Configures PWM on GPIO 18 (the pin that supports hardware PWM across all models).
3. Allows the user to select the carrier frequency, tone frequency, and sideband (USB or LSB).
4. Overlays a tone with a 200 Hz-wide passband on the carrier signal.

## Code Example

``` cpp
#include <iostream>
#include <cmath>
#include <pigpio.h>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <conio.h> // For _kbhit and _getch on Windows-like systems

// Enum to define sideband types
enum Sideband { USB, LSB };

// Global atomic flag to control when to stop PWM generation
std::atomic<bool> stopFlag(false);

// Function to get user input with a prompt
template <typename T>
T getInput(const std::string& prompt) {
    T value;
    std::cout << prompt;
    std::cin >> value;
    return value;
}

// Function to detect Raspberry Pi model
std::string getPiModel() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line, modelName;
    while (std::getline(cpuinfo, line)) {
        if (line.find("Model") != std::string::npos) {
            modelName = line.substr(line.find(":") + 2);
            break;
        }
    }
    return modelName;
}

// Function to generate a modulated PWM tone with USB or LSB selection
void generateTone(int pin, int carrierFreq, int toneFreq, int dutyCyclePercentage, Sideband sideband) {
    // Calculate duty cycle value (0 to 255 scale for pigpio)
    int maxDutyCycle = (dutyCyclePercentage * 255) / 100;

    // Set the PWM frequency to the carrier frequency
    gpioSetPWMfrequency(pin, carrierFreq);

    // Set up tone modulation within a 200 Hz passband
    double modPeriod = 1.0 / 200;  // 200 Hz modulation cycle for passband

    auto startTime = std::chrono::steady_clock::now();

    while (!stopFlag.load()) {
        auto currentTime = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - startTime).count();

        // Calculate the modulated duty cycle using a sine wave for USB or LSB
        double modulatedToneFreq = (sideband == USB) ? (toneFreq + 200 * sin(2.0 * M_PI * elapsed / modPeriod))
                                                     : (toneFreq - 200 * sin(2.0 * M_PI * elapsed / modPeriod));

        int modulatedDutyCycle = static_cast<int>(
            maxDutyCycle * (0.5 * (1.0 + sin(2.0 * M_PI * modulatedToneFreq * elapsed)))
        );

        gpioPWM(pin, modulatedDutyCycle);

        // Sleep for a short interval (1 ms) to reduce CPU load
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Function to monitor for spacebar press to stop the program
void monitorSpacebar() {
    while (!stopFlag.load()) {
        if (_kbhit()) { // Detects if a key was pressed
            char ch = _getch(); // Get the pressed key
            if (ch == ' ') { // Check if it was the spacebar
                stopFlag.store(true);
                std::cout << "\nSpacebar pressed. Exiting...\n";
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Avoid high CPU usage in this loop
    }
}

int main() {
    int pin = 18;  // Use GPIO 18 for hardware PWM (available on all Pi models)

    // Detect the Raspberry Pi model
    std::string piModel = getPiModel();
    std::cout << "Detected Raspberry Pi Model: " << piModel << std::endl;

    // Get user-selectable parameters
    int carrierFreq = getInput<int>("Enter carrier frequency in Hz (suggested < 1 MHz): ");
    int toneFreq = getInput<int>("Enter tone frequency in Hz (1400 - 1600 Hz): ");
    int dutyCyclePercentage = getInput<int>("Enter duty cycle percentage (0-100): ");

    std::string sidebandChoice;
    std::cout << "Select sideband (USB/LSB): ";
    std::cin >> sidebandChoice;

    Sideband sideband;
    if (sidebandChoice == "USB" || sidebandChoice == "usb") {
        sideband = USB;
    } else if (sidebandChoice == "LSB" || sidebandChoice == "lsb") {
        sideband = LSB;
    } else {
        std::cerr << "Invalid sideband choice. Please select USB or LSB." << std::endl;
        return 1;
    }

    // Initialize pigpio library
    if (gpioInitialise() < 0) {
        std::cerr << "pigpio initialization failed." << std::endl;
        return 1;
    }

    std::cout << "Starting " << (sideband == USB ? "Upper Sideband (USB)" : "Lower Sideband (LSB)")
              << " tone modulation on GPIO " << pin << " with carrier frequency: " << carrierFreq
              << " Hz, tone frequency: " << toneFreq << " Hz." << std::endl;
    std::cout << "Press the spacebar to exit." << std::endl;

    // Start the PWM tone generation in a separate thread
    std::thread toneThread(generateTone, pin, carrierFreq, toneFreq, dutyCyclePercentage, sideband);

    // Start the spacebar monitor in the main thread
    monitorSpacebar();

    // Wait for the tone generation thread to finish
    toneThread.join();

    gpioTerminate();  // Clean up pigpio resources when done
    return 0;
}
```

### Explanation of the Code

1. Model Detection:
    * The `getPiModel()` function reads `/proc/cpuinfo` to determine the Raspberry Pi model. This provides confirmation of the hardware being used, though it does not affect the code logic directly since GPIO 18 is compatible across all models.

2. User Input:
    * The program prompts the user for the carrier frequency, tone frequency (within 1400–1600 Hz), duty cycle percentage, and sideband type (USB or LSB). These values allow the user to dynamically configure the PWM output and modulation characteristics.

3. Tone Generation:
    * The `generateTone()` function sets the base carrier frequency using `gpioSetPWMfrequency()` and then modulates the duty cycle sinusoidally.
    * Sideband Modulation: The modulation frequency is adjusted based on the selected sideband (USB or LSB). USB adds 200 Hz, while LSB subtracts 200 Hz, giving a 200 Hz passband.
    * The modulation loop uses `std::chrono` to keep the timing consistent and periodically adjusts the PWM duty cycle.

4. Hardware PWM:
    * Using GPIO 18 ensures hardware PWM is used, allowing higher frequency outputs and better timing accuracy than software PWM, which is important for creating accurate tones and modulation.

5. Spacebar Monitoring:
    * The `monitorSpacebar()` function checks for a spacebar press using `_kbhit()` and `_getch()` from the `conio.h` library. When the spacebar is detected, it sets the `stopFlag` to `true`, signaling the program to exit.
    * The function runs in the main thread, while the `generateTone()` function continues running in a separate thread.

6. Stopping PWM Generation:
    * The `stopFlag` is an `std::atomic<bool>` variable shared between threads. It’s initially set to `false` and is set to `true` when the spacebar is pressed, causing `generateTone()` to stop its loop.

7. Multithreading:
    * The PWM generation function `generateTone()` runs in a separate thread (`toneThread`). This allows the main thread to monitor for the spacebar input without blocking the tone generation.

8. Termination:
    * When the spacebar is pressed, `monitorSpacebar()` sets `stopFlag` to true, which stops the tone generation thread. The `toneThread.join()` call waits for the PWM generation to end before terminating `pigpio`.

## Usage

1. Compile and Run:

    ``` bash
    g++ -o pwm_ssb pwm_ssb.cpp -lpigpio -lrt -lm
    sudo ./pwm_ssb
    ```

2. User Prompts:
    * The program will prompt you to enter:
        * **Carrier Frequency:** Suggested values up to 1 MHz for stable output.
        * **Tone Frequency:** Must be between 1400 and 1600 Hz.
        * **Duty Cycle:** Percentage from 0–100%.
        * **Sideband Type:** Enter `USB` for Upper Sideband or `LSB` for Lower Sideband.

3. End Program
    * Press the spacebar to stop the program. This triggers `stopFlag`, stopping PWM generation and cleaning up resources.

## Notes

* **Frequency Range:** Ensure the selected carrier frequency is within the Pi's PWM capability. Generally, 1 MHz is a stable maximum for most Raspberry Pi models using hardware PWM.
* **Hardware PWM:** This code uses `GPIO 18` for hardware PWM, ensuring compatibility across all Pi models.
* **SSB Approximation:** This program simulates an SSB signal by adjusting the frequency and duty cycle in real-time, but it is an approximation rather than true SSB modulation.
* **Cross-Platform Compatibility:** This solution assumes `_kbhit()` and `_getch()` are available (from `conio.h`). On Linux, additional libraries may be required for non-blocking keyboard input.
* **CPU Load Management:** The `monitorSpacebar` loop includes a short sleep (10 ms) to prevent high CPU usage while checking for key input.
