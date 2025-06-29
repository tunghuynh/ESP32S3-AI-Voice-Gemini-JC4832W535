# ESP32-S3 AI Voice Assistant

A comprehensive AI-powered voice assistant built on ESP32-S3 with touch display, featuring speech recognition, text-to-speech, and integration with Google Gemini AI.

## 🚀 Features

- **Voice Recognition**: Record audio via INMP441 microphone and convert to text
- **AI Chat Interface**: Integration with Google Gemini 2.0 Flash model
- **Text-to-Speech**: Audio output through MAX98357A amplifier
- **Touch Display**: 3.5" IPS display (480x320) with LVGL GUI
- **WiFi Connectivity**: Automatic connection with fallback credentials
- **Modern UI**: Responsive interface with chat history and settings

## 🛠 Hardware Requirements

### Main Components
- **ESP32-S3-N16R8V** (16MB Flash, 8MB PSRAM)
- **JC3248W535C Display Module** (3.5" IPS, 480x320, AXS15231B driver)
- **INMP441 Microphone** (I2S digital microphone)
- **MAX98357A Amplifier** (I2S audio output)

### Pin Configuration

#### Display (Parallel Interface)
- **CS**: GPIO 45
- **SCK**: GPIO 47  
- **SDA0**: GPIO 21
- **SDA1**: GPIO 48
- **SDA2**: GPIO 40
- **SDA3**: GPIO 39
- **Backlight**: GPIO 1

#### Touch (I2C)
- **SDA**: GPIO 4
- **SCL**: GPIO 8
- **INT**: GPIO 3
- **Address**: 0x3B

#### Microphone (I2S)
- **WS (LRC)**: GPIO 5
- **SD (DOUT)**: GPIO 6
- **SCK (BCLK)**: GPIO 7

#### Speaker (I2S)
- **DOUT**: GPIO 41
- **BCLK**: GPIO 42
- **LRC**: GPIO 2

## 📋 Software Dependencies

### PlatformIO Libraries
```ini
lib_deps = 
    moononournation/GFX Library for Arduino@^1.5.0
    lvgl/lvgl@^9.1.0
    bblanchon/ArduinoJson@^7.4.1
    esphome/ESP32-audioI2S@^2.3.0
```

### Required APIs
- **Google Gemini API**: For AI text generation
- **WiFi Network**: For internet connectivity

## 🔧 Setup Instructions

### 1. Hardware Assembly
1. Connect ESP32-S3 to JC3248W535C display module
2. Wire INMP441 microphone to I2S pins
3. Connect MAX98357A amplifier for audio output
4. Ensure proper power supply (5V recommended)

### 2. Software Configuration

#### Install PlatformIO
```bash
# Install PlatformIO Core
pip install platformio

# Or use PlatformIO IDE extension in VS Code
```

#### Clone and Build
```bash
git clone <repository-url>
cd JC3248W535_lvgl_test
pio run
```

#### Configuration Files
Update credentials in `src/gemini_client.cpp`:
```cpp
static const char *apiKey = "YOUR_GEMINI_API_KEY";
```

Update WiFi credentials in `src/wifi_manager.cpp`:
```cpp
const char *use_ssid = "YOUR_WIFI_SSID";
const char *use_pass = "YOUR_WIFI_PASSWORD";
```

### 3. Upload Firmware
```bash
pio run --target upload
```

## 🎯 Usage

### Voice Commands
1. **Touch to Talk**: Tap the designated area on screen (coordinates 100-220, 350-450)
2. **Speak**: Device records for 5 seconds automatically
3. **AI Response**: Gemini processes your speech and responds
4. **Audio Feedback**: Text-to-speech plays the AI response

### Settings Menu
- Configure WiFi credentials
- Set Gemini API key
- Adjust audio settings
- View system information

## 🏗 Project Structure

```
src/
├── main.cpp                 # Main application entry point
├── speech_to_text.c/h       # I2S microphone recording & STT
├── text_to_speech.cpp/h     # I2S audio output & TTS
├── gemini_client.cpp/h      # Google Gemini AI integration
├── wifi_manager.cpp/h       # WiFi connection management
├── ui_manager.cpp/h         # UI helper functions
├── storage_manager.c/h      # Configuration storage
├── AXS15231B_touch.cpp/h    # Touch controller driver
├── pincfg.h                 # Pin definitions
├── dispcfg.h                # Display configuration
└── gemini/                  # SquareLine Studio UI files
    ├── ui.c/h               # Main UI framework
    ├── ui_events.c/h        # UI event handlers
    ├── ui_helpers.c/h       # UI utility functions
    └── screens/             # UI screen definitions
        ├── ui_Main.c/h      # Main chat screen
        └── ui_Settings.c/h  # Settings screen
```

## 🔊 Audio Specifications

### Recording
- **Sample Rate**: 16 kHz
- **Bit Depth**: 16-bit PCM
- **Channels**: Mono
- **Duration**: 5 seconds per recording
- **Format**: WAV with proper headers

### Playback
- **Sample Rate**: 22.05 kHz
- **Bit Depth**: 16-bit PCM
- **Channels**: Mono
- **Amplifier**: MAX98357A I2S

## 🧠 AI Integration

### Gemini API Features
- **Model**: gemini-2.0-flash
- **Max Tokens**: 100 (configurable)
- **Temperature**: 0.7
- **UTF-8 Support**: Full Vietnamese language support
- **Response Time**: ~2-3 seconds

### Speech Processing
- **Voice Activity Detection**: Amplitude-based detection
- **Noise Filtering**: Basic audio level thresholding
- **Format**: WAV files compatible with most STT services

## 🔧 Configuration

### Build Flags
```ini
build_flags = 
    -D LV_CONF_INCLUDE_SIMPLE
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D BOARD_HAS_PSRAM
    -D CONFIG_SPIRAM_SPEED_80M=1
    -D CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384
    -D CONFIG_FREERTOS_HZ=1000
```

### Memory Configuration
- **LVGL Buffer**: 64KB (uses PSRAM when available)
- **Audio Buffer**: ~160KB for 5-second recording
- **Display Buffer**: Allocated in PSRAM
- **Stack Size**: 16KB for main task

## 🚨 Troubleshooting

### Common Issues

#### Audio Problems
- **No microphone input**: Check I2S pin connections
- **Distorted audio**: Verify power supply stability
- **No TTS output**: Ensure MAX98357A wiring is correct

#### Display Issues
- **Blank screen**: Check backlight connection (GPIO 1)
- **Touch not working**: Verify I2C connections and address
- **UI corruption**: Increase LVGL buffer size

#### WiFi Connection
- **Connection timeout**: Verify SSID/password
- **API errors**: Check Gemini API key validity
- **Slow response**: Ensure stable internet connection

### Debug Output
Enable debug logging in `platformio.ini`:
```ini
monitor_filters = esp32_exception_decoder
monitor_speed = 115200
```

## 📊 Performance Metrics

- **Boot Time**: ~3-5 seconds
- **Voice Recognition**: ~2-3 seconds processing
- **AI Response**: ~2-4 seconds (network dependent)
- **Memory Usage**: ~60% of available RAM
- **Power Consumption**: ~200-300mA during operation

## 🔒 Security Considerations

### Known Security Issues
⚠️ **WARNING**: This code contains hardcoded credentials and should be updated before deployment:

1. **API Key Exposure**: Gemini API key is hardcoded in source
2. **WiFi Credentials**: Default SSID/password in source code
3. **Buffer Overflows**: Fixed-size buffers in UI code
4. **Memory Leaks**: Potential issues in UI helper functions

### Recommendations for Production
- Use environment variables for sensitive data
- Implement proper credential management
- Add input validation and sanitization
- Conduct security audit before deployment

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- **LVGL**: Lightweight and versatile graphics library
- **SquareLine Studio**: UI design and code generation
- **Google Gemini**: AI language model integration
- **Arduino ESP32**: Hardware abstraction layer
- **PlatformIO**: Cross-platform build system

## 📞 Support

For issues and questions:
1. Check the troubleshooting section
2. Review existing issues in the repository
3. Create a new issue with detailed description
4. Include serial output and hardware configuration

---

**Project Status**: ✅ Fully Functional
**Last Updated**: December 2024
**Version**: 2.0.0