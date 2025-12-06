// #include "mainserver.h"
// #include <WiFi.h>
// #include <WebServer.h>

// bool led1_state = false;
// bool led2_state = false;
// bool isAPMode = true;

// WebServer server(80);

// unsigned long connect_start_ms = 0;
// bool connecting = false;

// String mainPage()
// {
//   float temperature = glob_temperature;
//   float humidity = glob_humidity;
//   String led1 = led1_state ? "ON" : "OFF";
//   String led2 = led2_state ? "ON" : "OFF";

//   return R"rawliteral(
//   <!DOCTYPE html>
//   <html lang="vi">
//   <head>
//     <meta charset="UTF-8">
//     <meta name="viewport" content="width=device-width, initial-scale=1.0">
//     <title>ESP32 Dashboard</title>
//     <style>
//       body {
//         font-family: "Segoe UI", Arial, sans-serif;
//         background: #f2f3f5;
//         color: #333;
//         text-align: center;
//         margin: 0;
//         height: 100vh;
//         display: flex;
//         flex-direction: column;
//         align-items: center;
//         justify-content: center;
//       }

//       .logo {
//         width: 220px;
//         height: 90px;
//         border-radius: 20px;
//         box-shadow: 0 4px 10px rgba(0,0,0,0.2);
//         margin-bottom: 25px;
//         background: #f2f3f5;
//         object-fit: contain;
//         padding: 10px 15px;
//       }

//       .container {
//         background: linear-gradient(135deg, #1e90ff, #00bfff);
//         padding: 35px 45px;
//         border-radius: 20px;
//         box-shadow: 0 4px 25px rgba(0,0,0,0.15);
//         width: 90%;
//         max-width: 400px;
//         color: #fff;
//         backdrop-filter: blur(6px);
//       }

//       h1 {
//         font-size: 1.8em;
//         margin-bottom: 20px;
//       }

//       .sensor {
//         font-size: 1.1em;
//         margin: 10px 0;
//       }

//       .sensor span {
//         font-weight: bold;
//         color: #00ffcc;
//       }

//       button {
//         margin: 10px;
//         background: #00ffcc;
//         color: #000;
//         font-weight: bold;
//         border: none;
//         border-radius: 20px;
//         padding: 10px 20px;
//         cursor: pointer;
//         transition: all 0.3s;
//         font-size: 1em;
//       }

//       button:hover {
//         background: #00e0b0;
//         transform: scale(1.05);
//       }

//       #settings {
//         background: #f2f3f5;
//         color: #007bff;
//         font-weight: bold;
//       }

//       #settings:hover {
//         background: #e6f3ff;
//       }
//     </style>
//   </head>

//   <body>
//     <div class="container">
//       <h1>📊 ESP32 Dashboard</h1>
//       <div class="sensor">
//         🌡️ Nhiệt độ: <span id="temp"></span> &deg;C
//       </div>
//       <div class="sensor">
//         💧 Độ ẩm: <span id="hum"></span> %
//       </div>

//       <div>
//         <button onclick='toggleLED(1)'>💡 LED1</button>
//         <button onclick='toggleLED(2)'>💡 LED2</button>
//       </div>

//       <button id="settings" onclick="window.location='/settings'">⚙️ Cài đặt</button>
//     </div>
//   </body>
//   </html>
//   )rawliteral";
// }

// // ========== Handlers ==========
// void handleRoot() { server.send(200, "text/html", mainPage()); }

// void handleToggle()
// {
//   int led = server.arg("led").toInt();
//   if (led == 1)
//   {
//     led1_state = !led1_state;
//     Serial.println("YOUR CODE TO CONTROL LED1");
//   }
//   else if (led == 2)
//   {
//     led2_state = !led2_state;
//     Serial.println("YOUR CODE TO CONTROL LED2");
//   }
//   server.send(200, "application/json",
//               "{\"led1\":\"" + String(led1_state ? "ON" : "OFF") +
//                   "\",\"led2\":\"" + String(led2_state ? "ON" : "OFF") + "\"}");
// }

// void handleSensors()
// {
//   float t = glob_temperature;
//   float h = glob_humidity;
//   String json = "{\"temp\":" + String(t) + ",\"hum\":" + String(h) + "}";
//   server.send(200, "application/json", json);
// }

// void handleSettings() { server.send(200, "text/html", settingsPage()); }

// void handleConnect()
// {
//   wifi_ssid = server.arg("ssid");
//   wifi_password = server.arg("pass");
//   server.send(200, "text/plain", "Connecting....");
//   isAPMode = false;
//   connecting = true;
//   connect_start_ms = millis();
//   connectToWiFi();
// }

// // ========== WiFi ==========
// void setupServer()
// {
//   server.on("/", HTTP_GET, handleRoot);
//   server.on("/toggle", HTTP_GET, handleToggle);
//   server.on("/sensors", HTTP_GET, handleSensors);
//   server.on("/settings", HTTP_GET, handleSettings);
//   server.on("/connect", HTTP_GET, handleConnect);
//   server.begin();
// }

// void startAP()
// {
//   WiFi.mode(WIFI_AP);
//   WiFi.softAP(ssid.c_str(), password.c_str());
//   Serial.print("AP IP address: ");
//   Serial.println(WiFi.softAPIP());
//   isAPMode = true;
//   connecting = false;
// }

// void connectToWiFi()
// {
//   WiFi.mode(WIFI_STA);
//   if (wifi_password.isEmpty())
//   {
//     WiFi.begin(wifi_ssid.c_str());
//   }
//   else
//   {
//     WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
//   }
//   Serial.print("Connecting to: ");
//   Serial.print(wifi_ssid.c_str());

//   Serial.print(" Password: ");
//   Serial.print(wifi_password.c_str());
// }

// // ========== Main task ==========
// void main_server_task(void *pvParameters)
// {
//   pinMode(BOOT_PIN, INPUT_PULLUP);

//   startAP();
//   setupServer();

//   while (1)
//   {
//     server.handleClient();

//     // BOOT Button to switch to AP Mode
//     if (digitalRead(BOOT_PIN) == LOW)
//     {
//       vTaskDelay(100);
//       if (digitalRead(BOOT_PIN) == LOW)
//       {
//         if (!isAPMode)
//         {
//           startAP();
//           setupServer();
//         }
//       }
//     }

//     // STA Mode
//     if (connecting)
//     {
//       if (WiFi.status() == WL_CONNECTED)
//       {
//         Serial.print("STA IP address: ");
//         Serial.println(WiFi.localIP());
//         isWifiConnected = true; // Internet access

//         xSemaphoreGive(xBinarySemaphoreInternet);

//         isAPMode = false;
//         connecting = false;
//       }
//       else if (millis() - connect_start_ms > 10000)
//       { // timeout 10s
//         Serial.println("WiFi connect failed! Back to AP.");
//         startAP();
//         setupServer();
//         connecting = false;
//         isWifiConnected = false;
//       }
//     }

//     vTaskDelay(20); // avoid watchdog reset
//   }
// }

