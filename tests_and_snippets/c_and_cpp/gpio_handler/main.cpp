/**
 * @file main.cpp
 * @brief Example program demonstrating GPIO handling with callback
 *        functionality.
 * @details This program initializes GPIO pins for an LED and a button,
 *          sets up a callback for edge detection on the button, and
 *          allows the user to exit by pressing Enter.
 */

 #include "lcblog.hpp"
 #include "gpio_handler.hpp"
 #include <iostream>
 #include <sstream>
 
 /**
  * @brief Global logger instance for system-wide logging.
  * @details Initializes the LCBLog instance to log messages to standard output
  * and error streams.
  */
 LCBLog llog(std::cout, std::cerr);
 
 /// @brief GPIO pin number for the LED output.
 constexpr int LED = 18;
 
 /// @brief GPIO pin number for the button input.
 constexpr int BUTTON = 19;
 
 /**
  * @brief Callback function for GPIO edge detection.
  * @details This function is triggered when an edge (rising or falling) is
  *          detected on the specified GPIO pin. It logs the event with the
  *          edge type and pin state.
  *
  * @param edge The type of edge detected (RISING or FALLING).
  * @param state The current state of the GPIO pin (HIGH or LOW).
  */
 void gpio_callback(GpioHandler::EdgeType edge, bool state)
 {
     std::ostringstream msg;
     msg << "GPIO Callback: Edge " << (edge == GpioHandler::EdgeType::RISING ? "RISING" : "FALLING")
         << ", State: " << (state ? "HIGH" : "LOW") << ".";
 
     // Log the detected edge and state.
     llog.logS(INFO, msg.str());
 }
 
 /**
  * @brief Main function to initialize GPIO handlers and run the program.
  * @details Initializes GPIO handlers for an LED output and a button input
  *          with edge detection. The program waits for the user to press
  *          Enter to exit.
  *
  * @return int Returns 0 upon successful execution.
  */
 int main()
 {
     // Set the logging level to INFO.
     llog.setLogLevel(INFO);
     llog.logS(INFO, "Starting main.");
 
     try
     {
         // Create GPIO handlers for the button (input) and LED (output).
         GpioHandler inputHandler(BUTTON, true, true, gpio_callback, std::chrono::milliseconds(200));
         GpioHandler outputHandler(LED, false, false);
 
         llog.logS(INFO, "GPIO handler instances created.");
         llog.logS(INFO, "Press Enter to exit.");
 
         // Wait for the user to press Enter.
         std::cin.get();
 
         llog.logS(INFO, "Exiting main.");
         return 0;
     }
     catch (const std::exception &e)
     {
         // Log any exceptions encountered during execution.
         llog.logE(FATAL, "Exception caught in main: ", e.what());
         return 1;
     }
 }
