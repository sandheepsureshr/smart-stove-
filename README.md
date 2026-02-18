

Smart Stove IoT (ESP32 + TFT + Servo)

This project is an ESP32-based smart stove controller that provides a digital cooking timer, adjustable flame levels (low, medium, and high), TFT display user interface, push-button navigation with debounce, pause and resume functionality, buzzer alerts, long-press reset, and servo-based gas knob control.
The user can set hours, minutes, and seconds, choose a flame level, and start cooking. During cooking, the timer counts down and the servo maintains the selected flame. When the timer reaches zero, the system automatically turns off the gas and alerts the user using the buzzer. The OK button supports both short press actions for confirm, pause, and resume, and a long press of three seconds for full system reset.
The hardware required includes an ESP32, SPI TFT display compatible with the TFT_eSPI library, servo motor, five push buttons, buzzer, and power supply. The project uses the TFT_eSPI and ESP32Servo libraries and can be uploaded using the Arduino IDE by selecting the ESP32 Dev Module and the correct COM port.
This system is designed for educational and prototype purposes only and should not be connected directly to a real gas stove without proper certified safety mechanisms. Future improvements may include mobile app control, temperature sensing, automatic flame optimization, cooking presets, and touchscreen interface.

