

Smart Stove (ESP32 + TFT + Servo)

This project is an ESP32-based smart stove controller that provides a digital cooking timer, adjustable flame levels (low, medium, and high), TFT display user interface, push-button navigation with debounce, pause and resume functionality, buzzer alerts, long-press reset, and servo-based gas knob control.
The user can set hours, minutes, and seconds, choose a flame level, and start cooking. During cooking, the timer counts down and the servo maintains the selected flame. When the timer reaches zero, the system automatically turns off the gas and alerts the user using the buzzer. The OK button supports both short press actions for confirm, pause, and resume, and a long press of three seconds for full system reset.
The hardware required includes an ESP32, SPI TFT display compatible with the TFT_eSPI library, servo motor, five push buttons, buzzer, and power supply. The project uses the TFT_eSPI and ESP32Servo libraries and can be uploaded using the Arduino IDE by selecting the ESP32 Dev Module and the correct COM port.
This system is designed for educational and prototype purposes only and should not be connected directly to a real gas stove without proper certified safety mechanisms. Future improvements may include mobile app control, temperature sensing, automatic flame optimization, cooking presets, and touchscreen interface.

⏱️ Digital cooking timer

🔥 Adjustable flame levels (LOW / MED / HIGH)

📺 TFT display UI

🔘 Button navigation with debounce

⏸️ Pause / Resume cooking

🔊 Buzzer feedback & long-press reset

⚙️ Servo-controlled gas knob

Designed for safe, automated cooking control using embedded systems.

🧠 Features
1. Cooking Timer

Set hours, minutes, seconds

Countdown runs during cooking

Automatically stops gas when time finishes

2. Flame Control

Three flame levels:

LOW

MED

HIGH

Controlled using servo motor angles

3. User Interface

TFT display with:

Time display

Flame indicator

Current system state

Blinking cursor for editing

4. Button Controls
Button	Function
UP	Increase value / go back
DOWN	Decrease value
LEFT / RIGHT	Move cursor / change flame
OK (short press)	Confirm / Pause / Resume
OK (long press – 3s)	Full system reset
5. Safety Behavior

Gas automatically turns OFF when:

Timer reaches zero

System reset triggered

Cooking finished

🛠️ Hardware Required

ESP32

TFT Display (SPI, supported by TFT_eSPI)

Servo Motor (for gas knob control)

Push Buttons ×5

Buzzer

Power supply & wiring

📦 Libraries Used

Install from Arduino Library Manager:

TFT_eSPI

ESP32Servo
