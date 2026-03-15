🔫 Open Laser Tag (ESP32 Arena System)

Welcome to Open Laser Tag, a fully open-source, highly scalable, and professional-grade laser tag system built on the ESP32 platform.
This project allows you to build custom laser tag blasters with OLED HUDs, tactile controls, and massive outdoor range, all tied together by a central "Cheap Yellow Display" (CYD) touch server that hosts a live, cyberpunk-themed web dashboard for up to 64 players.
The original intent was to create an easy to setup lasertag game to undercut vendors who charge exhorbitant prices for a game. This hopefullt allows more educator such as myself to have access to easily available laser tag.

✨ Key Features

Massive Scalability: Play backyard games (up to 10 players) using the CYD's built-in Wi-Fi Hub, or scale up to massive 64-player arena matches using a standard, offline Wi-Fi router.
Smart Networking: Blasters and servers automatically search for a central router. If one isn't found, they seamlessly fall back to local Hub mode. No internet connection is ever required.
Live Web Dashboard: The CYD Server hosts a mobile-responsive web dashboard. Game masters can change rules, start/stop matches, and view live K/D/A telemetry from their smartphones.
Esports Scoreboard Overlay: The web dashboard features a dynamic, full-screen overlay for laptops/TVs that automatically divides into quadrants based on active teams (perfect for spectator displays).
Extreme Range Optics: Utilizes a custom 5V-driven TSAL6100 IR circuit paired with a 20mm plano-convex lens for 50+ meter outdoor "sniper" range.
On-the-fly Configuration: Blasters feature a hidden configuration menu (via the rotary encoder) to change Gun IDs, Teams, and Roles without needing to reflash the code.

🛠️ Hardware Requirements

The system is broken down into two main components: The Blasters and The Server.

The Blaster (Gun) BOM

Microcontroller: ESP32 38-Pin DevKitC (WROOM-32)
Display: 1.3" SH1106 I2C OLED (White or Blue)
Controls: * KY-040 Rotary Encoder (For volume, reloading, and menus)
KW12 Microswitch (For the main trigger)
Audio: Passive Piezo Buzzer (5V)

IR Optics:

Transmitter: TSAL6100 (940nm 5mm LED) + 2N2222 Transistor + 20Ω Resistor and 1000ohm resistor
Receiver: TSOP38238 (38kHz) x 3
Lens: 20mm Plano-Convex Lens (Mounted exactly ~21.7mm from the IR LED die)

Power: 18650 Battery + V8 Battery Shield (1-way) + Master Toggle Switch

The Server (Command Center)

Microcontroller/Display: ESP32-2432S028R "Cheap Yellow Display" (2.8" TFT with Touch)
(Alternative): Standard ESP32 with a 2.2" ST7789 TFT display.
Network: Any cheap, standard Wi-Fi router (for 50+ player games).

🔌 Crucial Wiring & Hardware Notes

If you are building this from scratch, pay close attention to these engineering details:
The 5V Laser Circuit: The ESP32's 3.3V pins cannot provide enough power for outdoor IR range. You must route 5V directly from the battery shield to the TSAL6100 LED, using a 2N2222 transistor as a switch controlled by the ESP32 (protected by a 1kΩ base resistor).
Battery Voltage Divider: Because the 18650 shield regulates output to a flat 5V, the ESP32 cannot natively read the battery's decay. You must wire a 100kΩ/100kΩ voltage divider directly from the raw battery (B+ pad) to an ESP32 ADC pin (IO34) to get accurate battery percentages on the OLED.
Multiple IR Receivers: If you want to add sensors to a headband or the sides of the blaster, wire multiple TSOP38238 receivers in Parallel (All VCCs together, all GNDs together, all OUTs together).
Trigger Pull-ups: The KW12 Trigger microswitch connects to GND and an ESP32 GPIO pin. No external resistors are needed; the software uses INPUT_PULLUP.


💻 Software & Installation

This project is built using PlatformIO (via VS Code). Using the Arduino IDE is not recommended due to library dependency management.

Setup Instructions

Clone this repository and open the project folder in VS Code with the PlatformIO extension installed.
The platformio.ini file will automatically download the correct libraries (like LovyanGFX or TFT_eSPI / Adafruit_ILI9341 depending on your screen hardware).
Connect your ESP32 or CYD via USB.
Click Upload in PlatformIO.



📡 Network & Operation Guide

How to Start a Match

(Optional) Power the Router: If playing a massive game, plug in your dedicated LaserArena Wi-Fi router. (Do not plug it into the internet).
Boot the CYD Server: Plug the CYD into a USB power bank. It will search for the router for 10 seconds.
If it finds it, it joins and shows a green [LA] tag.
If not, it creates its own Wi-Fi bubble and shows a yellow [LAH] tag.

Boot the Blasters: Turn on the guns. They will automatically hunt for the network and sync with the CYD Server.
Join the Dashboard: Connect your Smartphone or Laptop to the Wi-Fi network (LaserArena or LaserTag_Hub).

⚠️ IMPORTANT: Accessing the Web Dashboard

Modern devices (especially MacBooks and iPhones) use Secure DNS or Apple Private Relay, which breaks custom local domain names like lasertagarena.game.
To guarantee access to the command center, type the raw IP address directly into your browser:
If using the Router [LA]: Go to http://192.168.1.100
If using the CYD Hub [LAH]: Go to http://192.168.4.1

Managing Teams

You can change a player's team in three different ways:
On the Gun: The player turns the rotary encoder while in the lobby.
On the CYD: The Game Master taps the player's ID on the physical CYD Scoreboard tab.
On the Web App: The Game Master taps the player's row on their smartphone dashboard.

3D prints

Included the F3D files in case you want to edit, the main body hosues all the goods, the extention helps to convert it into a full tagger with grip and nerf stock adapter.

📜 License

lasertagopen is open-source and licensed under the [GNU AFFERO GENERAL PUBLIC LICENSE
Version 3].
If you wish to use this project in a closed-source or proprietary
commercial product, please contact chengxinze96@gmail,com for a commercial license.
Build, modify, and dominate the arena!
