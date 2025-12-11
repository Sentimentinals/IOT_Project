# YoloUNO DHT20 Project

## Mô tả
Đây là phiên bản sử dụng **DHT20** (I2C sensor) thay vì DHT11.

## Sự khác biệt so với project DHT11 gốc

| Feature | DHT11 (Original) | DHT20 (This project) |
|---------|------------------|----------------------|
| Giao thức | 1-Wire (GPIO 6) | I2C (Address 0x38) |
| Độ chính xác nhiệt độ | ±2°C | ±0.5°C |
| Độ chính xác độ ẩm | ±5% | ±3% |
| Library | DHT sensor library | DHT20 library |
| Mutex I2C | Chỉ cho OLED | Cho cả DHT20 + OLED |

## Kết nối phần cứng

### DHT20 Sensor (I2C)
- **VDD** → 3.3V
- **SDA** → GPIO 8 (I2C SDA)
- **GND** → GND
- **SCL** → GPIO 9 (I2C SCL)

### OLED SSD1306 (I2C)
- **VCC** → 3.3V
- **GND** → GND
- **SDA** → GPIO 8 (I2C SDA)
- **SCL** → GPIO 9 (I2C SCL)

> **Lưu ý**: DHT20 và OLED SSD1306 đều dùng chung bus I2C nhưng có địa chỉ khác nhau:
> - DHT20: 0x38
> - OLED: 0x3C

## Build và Upload

```bash
# Build project
pio run

# Upload firmware
pio run -t upload

# Monitor Serial
pio device monitor
```

## Cấu trúc thư mục

```
YoloUNO_DHT20_Lab2/
├── src/
│   ├── main.cpp
│   ├── temp_humi_oled.cpp    # DHT20 + OLED display task
│   └── ...
├── include/
│   ├── temp_humi_oled.h      # DHT20 header (thay vì DHT11)
│   └── ...
├── lib/
│   ├── DHT20/                # DHT20 I2C library
│   └── ...
└── platformio.ini
```

## Semaphore I2C

Project sử dụng `xI2CMutex` để bảo vệ bus I2C khi:
1. Đọc dữ liệu từ DHT20
2. Ghi dữ liệu lên OLED SSD1306

```cpp
if (xI2CMutex != NULL && xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    // Truy cập I2C an toàn
    int status = dht20.read();
    // ...
    xSemaphoreGive(xI2CMutex);
}
```
