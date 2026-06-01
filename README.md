# RT_transcriber_ESP32

Real-time voice transcription displayed on an ESP32-connected SH1106 OLED.

Speak into your browser → Groq Whisper transcribes it → text appears on both the web UI and the OLED simultaneously.

---

## How it works

```
Browser mic → POST /transcribe → Groq Whisper API
                                        ↓
Browser UI  ←── WebSocket broadcast ───┤
ESP32 OLED  ←── WebSocket broadcast ───┘
```

The FastAPI server accepts audio chunks from the browser every ~2.5 seconds, sends them to Groq Whisper, and broadcasts the returned text over WebSocket to every connected client — the browser tab and the ESP32 at the same time.

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 DevKit V1 (WROOM) |
| Display | SH1106 1.3" OLED, 128×64px, I2C |

**Wiring**

| OLED Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

---

## Project structure

```
livevoice/
├── server/
│   ├── server.py              entry point — run this
│   ├── config.py              all settings (model, port, language)
│   ├── routes.py              HTTP + WebSocket endpoints
│   ├── transcriber.py         Groq Whisper wrapper
│   ├── websocket_manager.py   WebSocket connection pool
│   ├── requirements.txt
│   └── static/
│       └── index.html         browser UI (Tailwind + vanilla JS)
│
└── esp32/
    ├── platformio.ini
    ├── include/
    │   ├── wifi_config.h      ← only file you need to edit before flashing
    │   ├── display.h
    │   ├── text_buffer.h
    │   └── ws_client.h
    └── src/
        ├── main.cpp           entry point
        ├── display.cpp        SH1106 via U8g2
        ├── text_buffer.cpp    word-wrap + scrolling line buffer
        └── ws_client.cpp      WebSocket client (Links2004/WebSockets)
```

---

## Setup

### 1. Get a Groq API key

Free at [console.groq.com/keys](https://console.groq.com/keys).

### 2. Run the server

```bash
cd server
pip install -r requirements.txt
export GROQ_API_KEY="gsk_..."
python server.py
```

Open [http://localhost:8000](http://localhost:8000) in your browser.

### 3. Find your LAN IP

```bash
hostname -I | awk '{print $1}'
```

### 4. Edit `esp32/include/wifi_config.h`

```cpp
#define WIFI_SSID     "your_network"
#define WIFI_PASSWORD "your_password"
#define WS_HOST       "192.168.1.x"   // your LAN IP from step 3
#define WS_PORT       8000
#define WS_PATH       "/ws"
```

### 5. Flash the ESP32

```bash
cd esp32
pio run --target upload
pio device monitor       # watch boot logs
```

### 6. Verify both clients connected

```bash
curl http://192.168.1.x:8000/status
# → {"ok": true, "clients": 2}
```

`clients: 2` = browser + ESP32 both connected.

---

## Dependencies

### Server
| Package | Role |
|---|---|
| fastapi | web framework |
| uvicorn | ASGI server |
| groq | Whisper STT API |
| python-multipart | audio file upload parsing |

### ESP32
| Library | Role |
|---|---|
| olikraus/U8g2 | SH1106 OLED driver |
| links2004/WebSockets | WebSocket client |
| ottowinter/ESPAsyncWebServer-esphome | (retained from snake project) |
| esphome/AsyncTCP-esphome | (retained from snake project) |

---

## OLED display behaviour

Font `u8g2_font_6x10_tf` gives 21 characters × 4 lines at 128×64px.

Incoming text is word-wrapped into lines and pushed into an 8-line scrolling buffer. The screen always shows the 4 most recent lines. The newest line is drawn bold to highlight it. Long sentences wrap automatically at word boundaries.

---
## Configuration

All server settings are in `server/config.py`:

```python
groq_model = "whisper-large-v3-turbo"   # or "whisper-large-v3" for more accuracy
language   = "en"                        # or "" for auto-detect
port       = 8000
```