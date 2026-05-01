# Automatic-Cat-Food-Dispenser 

A simple but reliable automatic cat food dispenser built with an ESP32 microcontroller. It feeds your cat at fixed times throughout the day, stops feeding at night, and plays a short buzzer sound before# each meal so your cat learns when to expect food.

---

## What This Does

The dispenser runs on a schedule. By default it opens the food valve at 8:00 AM, 12:00 PM, 4:00 PM, and 8:00 PM — four times a day, spaced every 4 hours. After 9:00 PM, the system will not dispense food even if it is powered on, so your cat can sleep without interruption.

Each time a feeding is due:

1. The buzzer plays 3 short beeps. Over time your cat will learn this sound means food is coming.
2. The servo rotates from 90 degrees to 0 degrees, which opens the valve.
3. Food falls through the chute into the bowl for 3 seconds.
4. The servo rotates back to 90 degrees, closing the valve.

The time is kept accurate automatically. The ESP32 connects to your WiFi and pulls the correct time from an internet time server (NTP). You do not need a separate real-time clock module.

---

## The HTML Demo

Open the `index.html` file in any web browser. It shows an animated simulation of the dispenser: the servo rotates, the valve opens, food falls into the bowl, a cat walks over and eats, then leaves. You can press "Simulate Feed Cycle" to watch it happen. This is purely for demonstration and does not require the hardware to be connected.

---

## Hardware You Need

**1. ESP32 Development Board**

Any standard ESP32 dev board works. These cost around 3 to 5 USD and are widely available online. It handles the WiFi connection, keeps track of the schedule, and controls the servo and buzzer.

**2. DS3225MG High-Torque Servo**

This is the specific servo recommended for this project. It provides 25 kg/cm of torque at 6V, which is more than enough to reliably open and close a food valve even if kibble gets wedged in the mechanism. Do not use a small 9g servo for this. A stuck valve could mean your cat does not get fed or cannot stop being fed.

Alternative high-torque servos that also work:
- Savox SC-1268SG (32 kg/cm, more expensive but very durable)
- LewanSoul LDX-335 (35 kg/cm, good value for the torque)

All three are standard PWM servos and are compatible with this code without any changes.

**3. Active Buzzer (5V)**

A small cylindrical active buzzer. "Active" means it makes noise on its own when you apply voltage — you do not need to generate a tone in code. These cost less than 1 USD.

**4. Power Supply**

The servo needs a dedicated 5V power supply rated for at least 2A. Do not power the servo from the ESP32's onboard 5V or 3.3V pins. The current draw when the servo moves can crash the ESP32 or damage it. The ESP32 itself can be powered via USB from a phone charger.

**5. Wiring and Enclosure**

Some jumper wires and a small breadboard for prototyping. For a permanent installation, a project box and terminal connectors are recommended.

---

## Wiring

    ESP32 GPIO 13  -->  Servo Signal (orange or yellow wire)
    External 5V    -->  Servo VCC (red wire)
    GND (shared)   -->  Servo GND (black or brown wire)

    ESP32 GPIO 12  -->  Buzzer positive leg
    GND            -->  Buzzer negative leg

Make sure the GND of the external 5V supply is connected to the GND of the ESP32. This is called a "common ground" and is required for the servo signal to work correctly.

---

## Software Setup

**Step 1 — Install Arduino IDE**

Download it from https://www.arduino.cc/en/software and install it. The free version works fine.

**Step 2 — Add ESP32 Board Support**

In Arduino IDE, go to File > Preferences. In the "Additional Boards Manager URLs" box, paste this URL:

    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

Then go to Tools > Board > Boards Manager, search for "esp32" and install the package by Espressif Systems.

**Step 3 — Install the Servo Library**

Go to Sketch > Include Library > Manage Libraries. Search for "ESP32Servo" and install the one by Kevin Harrington. This is different from the standard Arduino servo library and is required for the ESP32.

**Step 4 — Open the Code**

Open the file `cat_feeder.ino` in Arduino IDE.

**Step 5 — Enter Your WiFi Details**

Near the top of the file, find these two lines and replace the placeholder text with your actual WiFi name and password:

    const char* WIFI_SSID     = "YourWiFiName";
    const char* WIFI_PASSWORD = "YourWiFiPassword";

**Step 6 — Set Your Timezone**

The code is set to Indian Standard Time (UTC+5:30) by default. If you are in a different timezone, change this line:

    const long  GMT_OFFSET   = 19800;

The number is your UTC offset in seconds. For example, UTC+3 would be 10800 (3 x 3600). For UTC-5 it would be -18000.

**Step 7 — Upload the Code**

Connect the ESP32 to your computer with a USB cable. In Arduino IDE, select the correct board under Tools > Board (usually "ESP32 Dev Module") and the correct COM port under Tools > Port. Then click the Upload button (the right-arrow icon).

**Step 8 — Open Serial Monitor**

After uploading, go to Tools > Serial Monitor and set the baud rate to 115200. You will see the ESP32 connecting to WiFi, syncing the time, and printing the feeding schedule. If something goes wrong, the error message will appear here.

---

## Adjusting the Feeding Schedule

Open `cat_feeder.ino` and find this section:

    const int FEED_HOURS[] = { 8, 12, 16, 20 };

These are the hours in 24-hour format when feeding happens. You can add or remove hours. For example, to feed at 7 AM, 1 PM, and 7 PM only:

    const int FEED_HOURS[] = { 7, 13, 19 };

The line below it, `NUM_FEED_SLOTS`, calculates the count automatically so you do not need to change it.

---

## Adjusting the Night Cutoff

Find this line:

    #define NO_FEED_AFTER_HOUR  21

Change 21 to whatever hour (in 24-hour format) you want the feeding to stop. Setting it to 22 means no feeding at or after 10 PM.

---

## Adjusting the Valve Open Time

Find this line:

    #define VALVE_OPEN_DURATION_MS  3000

The number is in milliseconds. 3000 means 3 seconds. Change it to 5000 for 5 seconds if your food is slow to fall, or 2000 for 2 seconds if your cat gets too much.

---

## How the Valve Works

The servo controls a physical gate or paddle inside the chute above the food bowl. When the servo is at 90 degrees, the valve is closed and food cannot pass through. When the servo rotates to 0 degrees, the gate moves out of the way and food falls by gravity into the bowl below.

After 3 seconds, the servo returns to 90 degrees and the gate closes again.

The DS3225MG servo has enough torque to force the gate open even if dried kibble is partially blocking it. Smaller servos may stall in this situation, which is why a high-torque servo is specifically recommended.

---

## Troubleshooting

**WiFi is not connecting**
Double-check the SSID and password. Make sure your router is on 2.4 GHz — the ESP32 does not support 5 GHz WiFi.

**Time is wrong**
Your GMT_OFFSET value may be incorrect. Check your UTC offset and convert it to seconds (hours x 3600).

**Servo is twitching or not moving smoothly**
The servo is probably being powered from the ESP32 pin instead of a separate supply. Move the servo's red wire to a dedicated 5V source.

**The valve closes but food is still falling**
The physical valve gate may not be sealing completely. Adjust the servo mount or the gate geometry so the gate fully covers the chute opening at 90 degrees. You can also try 85 degrees instead of 90 if the servo's calibration is slightly off — change the VALVE_CLOSED value in the code.

**Buzzer is not making sound**
Make sure you have an "active" buzzer, not a "passive" one. A passive buzzer requires a tone signal from code and will not work with this sketch as written.

---

## Project File List

    cat_feeder.ino    Main firmware — upload this to the ESP32
    index.html        Browser-based demo with animation, no hardware needed
    README.md         This file

---

## License

MIT License. Free to use, modify, and share for personal and commercial projects. No warranty is provided.
