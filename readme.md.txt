# Motion Activated LED strip with LDR and PIR sensors and Arduino

An embedded system project built to control an LED strip based on motion detection and ambient light using **PIR** and **LDR sensors**. This system intelligently adjusts its behavior based on the environment, ensuring efficient light control and minimal false triggers.

![Demo GIF](images/demo.gif)

## 🚀 Features
- **Motion-based lighting**: LED strip turns on when motion is detected.
- **Light detection**: LDR detects light using a moving average filter for smooth detection.
- **Debounce logic**: Avoids false motion detection through consistent PIR sensor readings.
- **Hysteresis**: Ensures stable light state near threshold values.
- **Watchdog timer**: Periodically verifies system status to avoid misbehaving due to glitches or external interference.
- **Lockout period**: Prevents the LDR from detecting the LED strip’s own light during operation.

## 🧠 How it Works

### 🌗 Light Detection (LDR)
- The system samples **3 LDR readings** at a time, calculates the **moving average**, and checks if the environment is bright or dark.
- **Threshold with hysteresis**: To avoid flickering, a threshold value is set with a **hysteresis range** to change the state only when the reading goes beyond a certain limit.
- **Buffer**: The LDR readings are stored in a **circular buffer**, ensuring real-time data processing.

### 🕵️‍♂️ Motion Detection (PIR)
- The system reads the **PIR sensor** in **debounced intervals** to prevent false motion triggers.
- **Motion count** is accumulated over several readings, ensuring valid motion is detected if more than half the readings detect motion.

### 💡 LED Control
- **LED strip turns on** when motion is detected, and turns off when the environment is bright.
- **Lockout period** prevents feedback from the LED strip, ensuring stable light control.
- **LED state** is only updated if the state has changed, reducing unnecessary writes to the LED control pin.

### ⏲️ Watchdog Timer
- A **watchdog function** runs periodically to check if the actual pin output for the LED matches the state of `ledsEnabled`. If there is a mismatch, it corrects the issue and logs the error.

## 🧰 Installation

1. Clone this repository to your local machine:
   ```bash
   git clone https://github.com/yourusername/smart-led-light-controller.git

2. Open your Arduino IDE and load the provided code onto your microcontroller.

3. Ensure you have the necessary sensors connected to the correct pins.


## Key Variables

Variable			Purpose
ldrReadings[]			Holds recent LDR values for moving average
ldrTotal			Sum of ldrReadings
ldrAverage			Smoothed LDR value
motionDetected			Flag indicating motion has been detected
ledsEnabled			Tracks the state of the LED strip
ledOnTime			Timestamp when the LED strip was turned on
LDR_CALIBRATION_READS		Number of readings during startup to set threshold

## Components Used

1. Arduino UNO/Nano
2. LDR
3. HC-SR501 PIR Sensor
4. 10k ohm resistor (x2)
5. 330 ohm resistor (x1)
6. 12 V adapter
7. 12 V LED Strip (Any strip would do the work)
8. IRFZ44N MOSFET