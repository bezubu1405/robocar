
# ESP32 WiFi Car Controller 🚗

A feature-rich **WiFi-controlled car** project utilizing an **ESP32** microcontroller. This project boasts a **modern web interface** accessible from any device with a web browser, offering **smooth motor ramping**, **light relay control**, and an **emergency stop** function. All WiFi and mDNS configurations can be managed conveniently **from your web browser**, with settings stored persistently.

---

## ✨ Features

* **WiFi AP + STA Mode:** Connect directly to the car's WiFi network (Access Point mode) or integrate it into your existing home WiFi network (Station mode) for broader accessibility.
* **Modern Web UI:** Control your car with an intuitive web-based interface featuring:
    * **Joystick:** Drag to control steering and movement (supports touch and mouse input).
    * **Speed Slider:** Precisely adjust the maximum speed of the motors.
    * **Light Toggle:** Switch an external light or accessory on/off via a dedicated relay.
    * **Emergency Stop:** A prominent red button for instantly halting all motor activity.
* **Smooth Motor Ramping:** Implements gradual acceleration and deceleration to prevent jerky starts and stops, ensuring smooth and precise control.
* **Light Relay Control:** Dedicated control for an external light or accessory connected via a relay.
* **Emergency Stop:** Provides an immediate and reliable way to stop the car in critical situations.
* **WiFi & mDNS Settings:** Configure WiFi credentials (AP & STA) and mDNS hostname directly from the web interface, eliminating the need for re-uploading code.
* **Captive Portal:** In Access Point mode, devices connecting to the car's WiFi will be automatically redirected to the control web page.
* **Persistent Settings:** All configured WiFi settings, mDNS hostname, and other preferences are stored in the ESP32's non-volatile memory (NVS), so they persist even after power cycles.

---

## 🏗️ Block Diagram

This block diagram illustrates the overall architecture and communication flow of the ESP32 WiFi Car Controller.

```mermid
graph TD
    A[Smartphone/PC<br>Web Browser] -- WiFi --> B[ESP32<br>Web Server & Logic]
    B -- PWM/GPIO Signals --> C1[Motor Driver 1<br>(H-Bridge)]
    B -- PWM/GPIO Signals --> C2[Motor Driver 2<br>(H-Bridge)]
    B -- GPIO --> D[Relay<br>(Light)]
    C1 -- Power --> E1[DC Motor 1]
    C2 -- Power --> E2[DC Motor 2]
    D -- Power --> F[Lights/Accessory]
```

**Explanation:**
* **Smartphone/PC Web Browser:** The user interface for controlling the car.
* **ESP32:** The central processing unit. It hosts the web server, handles WiFi communication (AP/STA), processes user input from the web UI, implements motor control logic (including ramping), and manages settings.
* **Motor Driver (H-Bridge):** Two independent BTS7960 motor drivers receive control signals (Enable, PWM for speed, Direction) from the ESP32 and provide the necessary power to drive the DC motors.
* **Relay (Light):** An electrical switch controlled by the ESP32, used to turn an external light or accessory on/off.
* **Motors:** The DC motors responsible for propelling the car.
* **Lights/Accessory:** The external lighting or other peripheral controlled by the relay.

---

## ⚙️ Default WiFi Credentials

Upon first boot or reset, the ESP32 will use these default settings:

* **Access Point (AP) Mode:**
    * **SSID:** `ORBITERBOOTHS`
    * **Password:** `12345678`
* **Station (STA) Mode:**
    * **SSID:** `AAIRAH5G`
    * **Password:** `qwerty123`
* **mDNS Hostname:** `esp32car.local`

**Note:** You can change these settings directly from the web interface after initial connection.

---

## 📍 Pinout

Ensure your components are wired correctly to the ESP32 according to the pin assignments below.

| Function            | GPIO Pin | Description                                    |
| :------------------ | :------- | :--------------------------------------------- |
| `MOTOR1_REN`        | `32`     | Motor 1 Right Enable (Motor Driver 1)          |
| `MOTOR1_LEN`        | `33`     | Motor 1 Left Enable (Motor Driver 1)           |
| `MOTOR1_RPWM`       | `25`     | Motor 1 Right PWM (Speed Control, Driver 1)    |
| `MOTOR1_LPWM`       | `26`     | Motor 1 Left PWM (Speed Control, Driver 1)     |
| `MOTOR2_REN`        | `27`     | Motor 2 Right Enable (Motor Driver 2)          |
| `MOTOR2_LEN`        | `14`     | Motor 2 Left Enable (Motor Driver 2)           |
| `MOTOR2_RPWM`       | `12`     | Motor 2 Right PWM (Speed Control, Driver 2)    |
| `MOTOR2_LPWM`       | `13`     | Motor 2 Left PWM (Speed Control, Driver 2)     |
| `LIGHT_RELAY_PIN`   | `2`      | Control pin for the external light relay       |

---

## 🚀 Getting Started

Follow these steps to set up and run your ESP32 WiFi Car.

1.  **Clone or Download** this repository to your local machine.
2.  **Open in PlatformIO** (VSCode with PlatformIO extension is highly recommended for a smooth development experience).
3.  **Install Libraries:** Ensure you have the following libraries installed in your Arduino IDE or PlatformIO project:
    * `ESPAsyncWebServer`
    * `Preferences`
    * `ESPmDNS`
    *(PlatformIO usually handles dependencies automatically, but manual installation might be needed if issues arise.)*
4.  **Connect your ESP32** board to your computer.
5.  **Upload** the `esp32_wifi_car.ino` sketch to your ESP32.
6.  **Connect to WiFi:**
    * **AP Mode:** Connect your smartphone or PC to the WiFi network named `ORBITERBOOTHS` with the password `12345678`.
    * **STA Mode:** Alternatively, after initial setup, you can configure the ESP32 to connect to your home WiFi network via the web UI.
7.  **Open your web browser:**
    * If in **AP Mode**, navigate to `http://192.168.4.1/`.
    * If the ESP32 is connected to your local network and mDNS is supported by your device, you can navigate to `http://esp32car.local/`.

---

## 🌐 Web Interface Guide

Once you access the web interface, you'll find:

* **Joystick:** Drag the joystick icon around the circular area to control the car's direction and movement. Moving it up/down controls forward/backward, and left/right controls turning.
* **Speed Slider:** Adjust this slider to set the maximum speed the motors will operate at. This provides granular control over the car's velocity.
* **Light Switch:** A toggle switch to turn the external light or accessory connected to the relay on or off.
* **Emergency Stop:** A large, prominent red button. Clicking this will immediately halt all motor activity, regardless of current joystick input.
* **Settings:** Access a dedicated section to change WiFi credentials (both AP and STA), update the mDNS hostname, and reboot the ESP32 to apply new settings.

---

## ✂️ Customization

This project is designed to be easily customizable:

* **Pin Assignments:** You can modify the GPIO pin assignments for motors and the light relay by editing the definitions at the top of the `esp32_wifi_car.ino` file.
* **User Interface:** The HTML, CSS, and JavaScript for the web UI are embedded within the `webpage` variable in the `esp32_wifi_car.ino` file. You can directly edit this variable to customize the look and feel or add new UI elements.
* **Add Features:** Extend the functionality by adding new web routes in the ESP32 code and corresponding UI elements in the `webpage` variable.

---

## Screen Shots
![webinterface](https://github.com/user-attachments/assets/ad44eaec-771e-48a9-ba32-8702e231aa0f)    ![settingpage](https://github.com/user-attachments/assets/6464663c-7497-4184-8fab-b06b70e4c13e)

## 📄 License
This project is open-source and distributed under the **MIT License**. Feel free to use, modify, and adapt this project for your own educational, hobby, or commercial purposes.

---

Enjoy building and controlling your WiFi car! 🚗✨
