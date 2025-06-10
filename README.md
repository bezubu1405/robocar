ESP32 WiFi Car Controller
This project allows you to control a car using an ESP32 over WiFi via a web-based joystick interface.

🚀 Getting Started
1. Upload Code to ESP32
Upload esp32_wifi_car.ino to your ESP32 using the Arduino IDE or another compatible uploader.

2. Power and Boot ESP32
Power the ESP32 with 5V (use a step-down converter if needed).

Power the motor driver (BTS7960) with 12V–24V, depending on your motor specs.

3. Connect via WiFi
By default, the ESP32 starts in Access Point (AP) mode.

SSID: ORBITERBOOTHS

Password: 12345678

You can later modify the code to connect to a local WiFi network (STA mode).

4. Access the Controller
Open your browser and go to:

cpp
Copy
Edit
http://192.168.4.1/
You'll see a control interface with:

A joystick to steer

A slider to control speed

🔌 Wiring Diagram

Component	ESP32 Pin

Motor 1 REN	32
Motor 1 LEN	33
Motor 1 RPWM	25
Motor 1 LPWM	26

Motor 2 REN	27
Motor 2 LEN	14
Motor 2 RPWM	12
Motor 2 LPWM	13

Light Relay	2

Be sure to match your wiring to these pins for proper operation.

🛠 Power Requirements
ESP32: 5V

BTS7960 Motor Driver: 12V–24V (based on motor specifications)

Use a step-down converter (e.g., buck converter) to power ESP32 from the main battery if needed.

📄 License
Feel free to use or modify for your own projects!


![webinterface](https://github.com/user-attachments/assets/3a64dc45-ddad-43ac-8e47-eeca2f03e439)     ![image](https://github.com/user-attachments/assets/f8a81a2e-adbb-44c8-8a39-cb2fe229b230)


