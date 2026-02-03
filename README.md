# RFID Access Control System with Touch Menu (ESP32) 🔐

This project implements an autonomous access control system using an **ESP32**, a **Wiegand RFID** reader (HID/Generic), and an **OLED** display.

Unlike traditional systems, this project eliminates physical buttons and utilizes the ESP32's capacitive **Touch** pins to navigate a configuration menu, allowing users to be added and removed without the need for a PC.

## 📋 Features
* **Local Storage:** User database is saved in Flash memory (`LittleFS`) and remains persistent after reboots.
* **Touch Interface:** Menu navigation using capacitive `Touch` pins.
* **User Management:**
    * Read Mode (Access Granted/Denied).
    * Add User Mode (New ID).
    * Delete User Mode (Delete ID).
* **Security:** Access to the configuration menu is protected by a **Master Card**.

## 🛠️ Required Hardware
* **ESP32** Development Board (DevKit V1 or similar).
* RFID Reader with **Wiegand** protocol (D0/D1).
* I2C OLED Display (128x64) with **SSD1306** driver.
* Jumper wires.
* *(Optional)* Logic Level Converter if the reader data lines operate at 5V.

## 🔌 Wiring Diagram (Pinout)

Based on the configuration in `main.ino`:

### 1. RFID Reader (Wiegand)
| Reader Pin | ESP32 Pin (Code) | Note |
| :--- | :--- | :--- |
| **D0 (Green)** | **GPIO 18** | Data 0 |
| **D1 (White)** | **GPIO 19** | Data 1 |
| GND | GND | Common Ground |
| VCC | 12V / 5V | External Power Source |

> ⚠️ **Warning:** The ESP32 operates at 3.3V. If your RFID reader sends data at 5V, it is highly recommended to use a voltage divider or a Logic Level Converter on pins 18 and 19 to prevent damage to the microcontroller.

### 2. OLED Display (I2C)
| OLED Pin | ESP32 Pin |
| :--- | :--- |
| **SDA** | **GPIO 21** |
| **SCL** | **GPIO 22** |
| VCC | 3.3V / 5V |
| GND | GND |

### 3. Touch Controls
No physical buttons are required. Simply connect a loose wire or a metal plate to these pins:

| Function | ESP32 Pin (Code) | Action |
| :--- | :--- | :--- |
| **Up** | **GPIO 13** | Move cursor up |
| **Down** | **GPIO 4** | Move cursor down |
| **Select** | **GPIO 15** | Enter / Accept |

---

## 💾 Software Installation

### 1. Required Libraries
Install the following libraries via the Arduino IDE Library Manager:
* `Adafruit GFX Library`
* `Adafruit SSD1306`

### 2. IDE Configuration
To ensure the file system works correctly:
1.  Select your board (e.g., DOIT ESP32 DEVKIT V1).
2.  Go to **Tools > Partition Scheme** and select an option that allocates space for files, such as: **"Default 4MB with spiffs"** or **"No OTA (Large APP)"**.

### 3. Upload Code
Upload the `.ino` file to your board.

---

## 📖 Usage Instructions

### Normal Mode (Access Mode)
1.  The system starts by showing "SISTEMA ACTIVO" (System Active).
2.  Scan an RFID card.
3.  If the ID is in the database: **"ACCESO PERMITIDO"** (Access Granted).
4.  If not: **"ACCESO DENEGADO"** (Access Denied).

### Admin Menu
To enter the menu, you must scan the **Master Card**.
* **Default Master ID:** `134440` (You can change this in the code variable `const unsigned long ID_MAESTRA`).

**Navigation:**
* Touch **Pin 13** wire to move **Up**.
* Touch **Pin 4** wire to move **Down**.
* Touch **Pin 15** wire to **Select**.

**Menu Options:**
1.  **New ID:** Select this option and scan a new card to save it to memory.
2.  **Delete ID:** Select this option and scan an existing card to remove it.
3.  **Exit:** Return to normal mode.

## 🐛 Troubleshooting
* **Touch triggers automatically:** Adjust the `UMBRAL_TOUCH` (Threshold) variable in the code. If it's too sensitive, decrease the value (e.g., to 12). If it doesn't detect touches, increase it (e.g., to 25).
* **LittleFS Fail Error:** Make sure you have selected the correct **Partition Scheme** in the Arduino IDE before uploading the code.

---
**Author:** [Joseph Alexis Malpartida Candia / Jmalpartida42]
