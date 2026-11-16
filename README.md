# Automatic-Cat-Food-Dispenser 

An ESP32-based automatic cat-feeder prototype. It schedules a servo-operated gate and buzzer, with default feed times and a nightly cutoff. The current design has important failure modes and is not a verified unattended feeding appliance.

---

## What This Does

The dispenser runs on a schedule. By default it opens the food valve at 8:00 AM, 12:00 PM, 4:00 PM, and 8:00 PM — four times a day, spaced every 4 hours. After 9:00 PM, the system will not dispense food even if it is powered on, so your cat can sleep without interruption.

Each time a feeding is due:

1. The buzzer plays 3 short beeps. Over time your cat will learn this sound means food is coming.
2. The servo rotates from 90 degrees to 0 degrees, which opens the valve.
3. Food falls through the chute into the bowl for 3 seconds.
4. The servo rotates back to 90 degrees, closing the valve.

The firmware attempts to obtain time from an internet NTP server over Wi-Fi; it does not include a separate real-time clock. If local time is unavailable, the loop reports an error and skips dispensing while `getLocalTime()` continues to fail. A successful startup message alone is not proof that scheduled feeding will remain reliable.

---

## The HTML Demo

Open the `index.html` file in any web browser. It shows an animated simulation of the dispenser: the servo rotates, the valve opens, food falls into the bowl, a cat walks over and eats, then leaves. You can press "Simulate Feed Cycle" to watch it happen. This is purely for demonstration and does not require the hardware to be connected.

---

## Hardware You Need

**1. ESP32 Development Board**  <img width="232" height="234" alt="image" src="https://github.com/user-attachments/assets/36f7b008-c92b-4d32-a709-bccec2be9428" />


Any standard ESP32 dev board works. These cost around 3 to 5 USD and are widely available online. It handles the WiFi connection, keeps track of the schedule, and controls the servo and buzzer.

**2. DS3225MG High-Torque Servo** <img width="297" height="274" alt="image" src="https://github.com/user-attachments/assets/7509546e-f550-40c1-8532-85ac7dcf715f" />


Select a servo based on the actual gate load and the servo manufacturer's voltage and stall-current specifications. This repository does not validate servo torque, jam behavior, or compatibility of alternative models. Bench-test the linkage and gate repeatedly before adding food; do not assume that a listed servo can safely force a jammed mechanism.

**3. Active Buzzer (5V)**

A small cylindrical active buzzer. "Active" means it makes noise on its own when you apply voltage — you do not need to generate a tone in code. These cost less than 1 USD.

**4. Power Supply**

The wiring example uses a separate 5V servo supply, but verify the exact servo's permitted voltage and stall current before connecting it. Size the supply and wiring for that load. Do not power the servo from an ESP32 GPIO or 3.3V rail. Join the supply ground to ESP32 GND so the PWM signal has a common reference.

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

Create a local sketch folder named `cat_feeder`, copy `cat_feeder.ino` into it, and open that local sketch in Arduino IDE. This does not rename or move the repository's tracked files.

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

Change 21 to the cutoff hour (24-hour format). The current logic checks `NO_FEED_AFTER_HOUR`; the separate `LAST_FEED_HOUR` constant is defined in the sketch but is not used by the current scheduling logic.

---

## Adjusting the Valve Open Time

Find this line:

    #define VALVE_OPEN_DURATION_MS  3000

The number is in milliseconds. 3000 means 3 seconds. Change it to 5000 for 5 seconds if your food is slow to fall, or 2000 for 2 seconds if your cat gets too much.

---

## How the Valve Works

The servo controls a physical gate or paddle inside the chute above the food bowl. When the servo is at 90 degrees, the valve is closed and food cannot pass through. When the servo rotates to 0 degrees, the gate moves out of the way and food falls by gravity into the bowl below.

After 3 seconds, the servo returns to 90 degrees and the gate closes again.

The firmware does not sense a jam or confirm that the gate fully opened or closed. Gate geometry, servo torque, supply voltage, and food flow must be validated on the actual mechanism; the timed movement alone does not prove a successful or safe feed.

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

## First-run safety checklist

1. Test with the food hopper empty and the gate disconnected or safely unloaded. Keep fingers clear of the servo linkage; the servo moves to its closed angle during startup.
2. Confirm the servo's rated supply voltage and stall current before powering it. Use a separate, appropriately rated supply and a common ground.
3. Verify the Wi-Fi connection, NTP time, timezone, and printed feeding schedule in Serial Monitor at 115200 baud.
4. Measure actual portions over repeated trials before using the mechanism with food. A three-second opening is a time setting, not a calibrated portion measurement.
5. Do not leave the prototype as the animal's sole food source or unattended until power-loss, Wi-Fi/NTP failure, jammed-gate, over-dispense, and restart scenarios have been tested.

## Known limitations

- There is no dedicated RTC. The firmware depends on Wi-Fi/NTP time; when time cannot be read, scheduled dispensing is skipped.
- The last-feed guard is held in RAM rather than persisted. A reset around a scheduled minute can cause a feed to be repeated.
- There is no feedback for food quantity, an empty hopper, a jam, or the physical gate position. The firmware cannot prove that a portion was dispensed or that the valve sealed.
- `index.html` is a standalone browser animation, not a live dashboard connected to the ESP32.
- The repository contains `gitignore (2)`, not a standard `.gitignore` filename, and no root `LICENSE` file is present. The README's MIT statement should be backed by an actual license file before redistribution.
- Firmware and hardware were not compiled or tested during this documentation review.

## Project File List

    cat_feeder.ino    Main firmware — upload this to the ESP32
    index.html        Browser-based demo with animation, no hardware needed
    README.md         This file

---

## License

The README states that the project is MIT-licensed, but no `LICENSE` file is present in the current root tree. Confirm the intended license and add a standard license file before redistributing the project.


---

## Skills & technologies

`Arduino/C++` · `HTML/CSS`


## Repository structure

Key files and folders (not exhaustive):

- `README.md`
- `cat_feeder.ino`
- `gitignore (2)`
- `index.html`


## How it works

The main firmware sketch is `cat_feeder.ino`. Open it in Arduino IDE, verify the board, pin mapping and library requirements, then compile and upload. Test hardware behaviour on the real circuit before relying on it.


## Maintenance & Audit

- **Audit date:** 16 November 2026
- **Documentation status:** README reviewed and audit record added on this date.
- **Scope and evidence:** This is a documentation-level maintenance pass. It does not establish that source code compiles, dependencies are current, security has been tested, or hardware/runtime behavior has been verified.
- **Code and tests:** No new functional code changes or test results are claimed by this README update. Run project-specific build, test, and hardware/runtime checks before relying on the project.
- **Maintenance baseline:** Documentation status recorded through 16 November 2026; revisit setup instructions, dependencies, known limitations, and tests when the project changes.
