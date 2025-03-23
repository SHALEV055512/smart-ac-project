# Smart AC Automation Project

An intelligent air conditioning control system powered by **Arduino**, **Broadlink**, and **Google Sheets**, designed for smart automation, real-time logging, and remote control.

---

## 🧠 Overview
This project enables full control over an air conditioner using temperature and flame sensors connected to an Arduino. The system automates decision-making (on/off, temperature setpoints), communicates with a Flask backend, and logs data to a dynamically created Google Sheet.

---

## ⚙️ Technologies Used

- **Arduino (C++)** – Reads temperature, detects fire, sends HTTP requests
- **Flask (Python)** – API for receiving data and triggering IR commands
- **Broadlink RM Mini 3** – Sends IR signals to control the AC remotely
- **Google Sheets API** – Logs sensor data dynamically into weekly spreadsheets
- **gspread + oauth2client** – Python libraries for Google Sheets integration
- **NTPClient (Arduino)** – Gets accurate time via the internet

---

## 📁 Folder Structure

```
smart-ac-project/
├── arduino/                    # Arduino code for sensors & logic
│   └── Connect_to_wifi.ino
├── backend/                    # Python Flask server & logic
│   ├── broadlink_controller.py
│   └── google_sheets_logger.py
├── credentials/               # Google API credentials (NOT pushed to GitHub)
│   └── credentials.json       # <-- This file is ignored via .gitignore
└── .gitignore
```

---

## 🚀 How to Run

### 1. Google Sheets Setup
- Create a new Google Cloud project
- Enable **Google Sheets API** + **Google Drive API**
- Create a service account + download the `credentials.json` file
- Share the target Google Sheet with the service account's email

### 2. Python Dependencies
```bash
pip install flask gspread oauth2client schedule pytz broadlink
```

### 3. Run the Flask Backend
```bash
cd backend
python broadlink_controller.py     # Port 8000 - IR Control API
python google_sheets_logger.py     # Port 8001 - Data Logging API
```

### 4. Upload Arduino Code
- Connect Arduino to your computer
- Open `Connect_to_wifi.ino` in Arduino IDE
- Upload it after entering your Wi-Fi credentials and server IP

---

## 🔥 Features
- Smart decision logic based on outside/inside temperature
- Daily automation (turn AC on/off, adjust temp)
- Fire detection overrides all commands and shuts down AC
- Logs every data entry to a structured, color-coded Google Sheet
- Night/day classification for better AC behavior

---

## 📸 Demo Snapshot
*Coming Soon – Sheet screenshot or live demo link here*

---

## 🤝 Contributing
Pull requests are welcome! Feel free to fork the project and enhance it.

---

## 🛡️ Security Note
> The file `credentials/credentials.json` is **ignored** and should never be pushed to GitHub. Make sure to keep your service account private.

---

## 📬 Contact
Built with 💡 by **Shalev** — For questions or collaborations, feel free to reach out via GitHub!

