#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "time.h"
#include <FS.h>  // SPIFFS
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 12600, 60000); // Iran timezone offset = 3.5 hrs = 12600 sec

const char* ssid = "********";
const char* password = "********";

unsigned long previousMillis = 0;
const unsigned long interval = 60000;  // 60 seconds
unsigned long lastCheck = 0;

ESP8266WebServer server(80);

#define RELAY_PIN 2  // Change to your actual relay GPIO pin

void checkSchedule() {
  File file = SPIFFS.open("/schedule.json", "r");
  if (!file) {
    Serial.println("Failed to open schedule.json");
    return;
  }

  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size + 1]);
  file.readBytes(buf.get(), size);
  buf[size] = '\0';

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  String nowTime = timeClient.getFormattedTime().substring(0, 5); // HH:MM
  int dayIndex = timeClient.getDay();
  String days[] = {"su", "mo", "tu", "we", "th", "fr", "sa"};
  String today = days[dayIndex];

  for (JsonObject entry : doc.as<JsonArray>()) {
    String day = entry["day"];
    String time = entry["time"];
    String state = entry["state"];

    if (day == today && time == nowTime) {
      if (state == "on") {
        digitalWrite(RELAY_PIN, HIGH);
        Serial.println("Relay ON (Scheduled)");
      } else {
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("Relay OFF (Scheduled)");
      }
    }
  }
}

void handleRelayControl() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      digitalWrite(RELAY_PIN, HIGH);  // Turn relay ON
      server.send(200, "text/plain", "Relay ON");
    } else if (state == "off") {
      digitalWrite(RELAY_PIN, LOW);   // Turn relay OFF
      server.send(200, "text/plain", "Relay OFF");
    } else {
      server.send(400, "text/plain", "Invalid state");
    }
  } else {
    server.send(400, "text/plain", "Missing state argument");
  }
}

// List files as JSON
void handleFileList() {
  String path = "/";
  Dir dir = SPIFFS.openDir(path);
  String json = "[";
  while (dir.next()) {
    if (json != "[") json += ",";
    json += "{\"name\":\"" + dir.fileName() + "\",\"size\":" + String(dir.fileSize()) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

// Upload file (multipart/form-data)
void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    Serial.print("Upload Start: "); Serial.println(filename);
    File fsUploadFile = SPIFFS.open(filename, "w");
    fsUploadFile.close();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File fsUploadFile = SPIFFS.open("/" + upload.filename, "a");
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
      fsUploadFile.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.print("Upload End: "); Serial.println(upload.totalSize);
    server.sendHeader("Location", "/");  // Redirect after upload
    server.send(303);
  }
}

// Delete a file by name
void handleFileDelete() {
  if (server.hasArg("filename")) {
    String filename = server.arg("filename");
    if (SPIFFS.exists(filename)) {
      SPIFFS.remove(filename);
      server.send(200, "text/plain", "File deleted");
    } else {
      server.send(404, "text/plain", "File not found");
    }
  } else {
    server.send(400, "text/plain", "Filename not specified");
  }
}

// Download file
void handleFileDownload() {
  if (!server.hasArg("filename")) {
    server.send(400, "text/plain", "Filename not specified");
    return;
  }
  String filename = server.arg("filename");
  if (!SPIFFS.exists(filename)) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  File file = SPIFFS.open(filename, "r");
  server.streamFile(file, "application/octet-stream");
  file.close();
}




void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n", "IRDT-3:30");
  setenv("TZ", "IRDT-3:30", 1);
  tzset();
}

void initTime(String timezone){
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setTimezone(timezone);
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Failed to obtain time";
  }
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  return String(timeString);
}

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>ESP32 Time</title>
      <meta charset="UTF-8">
      <style>
        body { font-family: sans-serif; text-align: center; margin-top: 50px; }
        #time { font-size: 2em; color: #333; }
      </style>
    </head>
    <body>
      <h1>Current Time in Iran</h1>
      <div id="time">Loading...</div>
      <script>
        async function updateTime() {
          try {
            const res = await fetch('/api/time');
            const data = await res.json();
            document.getElementById('time').textContent = data.time;
          } catch (e) {
            document.getElementById('time').textContent = 'Error fetching time';
          }
        }
        setInterval(updateTime, 1000); // Update every second
        updateTime(); // Run immediately
      </script>

      <button id="fileManagerBtn">Open File Manager</button>

      <div id="fileManager" style="display:none; margin-top:20px;">
        <h2>SPIFFS File Manager</h2>
        <input type="file" id="uploadFile" />
        <button onclick="uploadFile()">Upload</button>
        <button onclick="refreshFiles()">Refresh List</button>

        <ul id="fileList"></ul>
      </div>

      <script>
        const fileManagerBtn = document.getElementById('fileManagerBtn');
        const fileManager = document.getElementById('fileManager');
        const uploadFileInput = document.getElementById('uploadFile');
        const fileList = document.getElementById('fileList');

        fileManagerBtn.addEventListener('click', () => {
          fileManager.style.display = fileManager.style.display === 'none' ? 'block' : 'none';
          if (fileManager.style.display === 'block') refreshFiles();
        });

        async function refreshFiles() {
          const res = await fetch('/api/files');
          const files = await res.json();
          fileList.innerHTML = '';
          files.forEach(f => {
            const li = document.createElement('li');
            li.textContent = `${f.name} (${f.size} bytes)`;
            const dl = document.createElement('button');
            dl.textContent = 'Download';
            dl.onclick = () => { window.open(`/api/download?filename=${encodeURIComponent(f.name)}`, '_blank'); };
            const del = document.createElement('button');
            del.textContent = 'Delete';
            del.onclick = async () => {
              if (confirm(`Delete ${f.name}?`)) {
                await fetch('/api/delete', {
                  method: 'POST',
                  headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                  body: `filename=${encodeURIComponent(f.name)}`
                });
                refreshFiles();
              }
            };
            li.appendChild(dl);
            li.appendChild(del);
            fileList.appendChild(li);
          });
        }

        async function uploadFile() {
          if (uploadFileInput.files.length === 0) {
            alert('Select a file first!');
            return;
          }
          const file = uploadFileInput.files[0];
          const formData = new FormData();
          formData.append('file', file);

          const res = await fetch('/api/upload', {method: 'POST', body: formData});
          if (res.ok) {
            alert('Upload successful');
            uploadFileInput.value = '';
            refreshFiles();
          } else {
            alert('Upload failed');
          }
        }
      </script>


      <h3>Relay Control</h3>
      <button onclick="relayControl('on')">Turn Relay ON</button>
      <button onclick="relayControl('off')">Turn Relay OFF</button>

      <p id="relayStatus"></p>

      <script>
        async function relayControl(state) {
          const response = await fetch('/relay', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: `state=${state}`
          });

          const text = await response.text();
          document.getElementById('relayStatus').innerText = text;
        }
      </script>

    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}



void setup() {
  Serial.begin(115200);
  SPIFFS.begin();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Relay OFF initially (or HIGH, depends on your relay)

  timeClient.begin();

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  initTime("IRDT-3:30");  // Iran timezone
  server.on("/", handleRoot);
  server.on("/api/time", HTTP_GET, []() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char timeStr[32];
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
      server.send(200, "application/json", String("{\"time\": \"") + timeStr + "\"}");
    } else {
      server.send(500, "application/json", "{\"error\": \"Failed to get time\"}");
    }
  });
  server.on("/api/files", HTTP_GET, handleFileList);
  server.on("/api/upload", HTTP_POST, []() {
    server.send(200);
  }, handleFileUpload);
  server.on("/api/delete", HTTP_POST, handleFileDelete);
  server.on("/api/download", HTTP_GET, handleFileDownload);
  // Existing server handlers...
  server.on("/relay", HTTP_POST, handleRelayControl);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  timeClient.update();
  if (millis() - lastCheck > 1000) {
    lastCheck = millis();
    checkSchedule();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.printf("Current time: %02d:%02d:%02d, Date: %04d-%02d-%02d\n",
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    } else {
      Serial.println("Failed to obtain time");
    }
  }
}