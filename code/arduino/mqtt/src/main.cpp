#include <Arduino.h>   

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ============================================================
// DEVICE CONFIGURATION
// ============================================================
const char* DEVICE_ID = "ESP567";
const char* FIRMWARE_VERSION = "1.0.0";


// ============================================================
// WIFI CONFIGURATION
// ============================================================
const char* WIFI_SSID = "Azmon Gostar";
const char* WIFI_PASSWORD = "agfaagfa1388";


// ============================================================
// MQTT CONFIGURATION
// ============================================================
const char* MQTT_SERVER = "192.168.1.15";
const uint16_t MQTT_PORT = 1883;


// ============================================================
// MQTT TOPICS
// ============================================================
const char* TOPIC_DATA = "home/ESP567/data";
const char* TOPIC_STATUS = "home/ESP567/status";
const char* TOPIC_CONTROL = "home/ESP567/control";
const char* TOPIC_RESPONSE = "home/ESP567/response";


// ============================================================
// TIMING
// ============================================================

const unsigned long TELEMETRY_INTERVAL = 2000;
const unsigned long WIFI_RECONNECT_INTERVAL = 5000;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;


// ============================================================
// OBJECTS
// ============================================================

WiFiClient espClient;

PubSubClient mqttClient(espClient);


// ============================================================
// STATE
// ============================================================
unsigned long lastTelemetryTime = 0;
unsigned long lastWiFiReconnectAttempt = 0;
unsigned long lastMQTTReconnectAttempt = 0;


// ============================================================
// LED
// ============================================================
bool ledState = false;


// ============================================================
// WIFI
// ============================================================
void connectWiFi();

// ============================================================
// WIFI STATUS
// ============================================================
void checkWiFi();

// ============================================================
// MQTT STATUS MESSAGE
// ============================================================
void publishStatus(const char* status);

// ============================================================
// MQTT CONNECT
// ============================================================
bool connectMQTT();


// ============================================================
// LED CONTROL
// ============================================================
void setLED(bool state);

// ============================================================
// COMMAND RESPONSE
// ============================================================
void publishCommandResponse(const char* commandId, const char* command, bool success, const char* message);

// ============================================================
// MQTT CALLBACK
// ============================================================
void callback(char* topic, byte* payload, unsigned int length);

// ============================================================
// TELEMETRY
// ============================================================
void publishTelemetry();


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(
        115200
    );

    delay(100);

    Serial.println();
    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "ESP8266 IoT Device"
    );

    Serial.print(
        "Device ID: "
    );

    Serial.println(
        DEVICE_ID
    );

    Serial.print(
        "Firmware: "
    );

    Serial.println(
        FIRMWARE_VERSION
    );

    Serial.println(
        "================================"
    );


    // --------------------------------------------------------
    // LED
    // --------------------------------------------------------

    pinMode(
        LED_BUILTIN,
        OUTPUT
    );

    setLED(
        false
    );


    // --------------------------------------------------------
    // Random seed
    // --------------------------------------------------------

    randomSeed(
        micros()
    );


    // --------------------------------------------------------
    // MQTT
    // --------------------------------------------------------

    mqttClient.setServer(
        MQTT_SERVER,
        MQTT_PORT
    );

    mqttClient.setCallback(
        callback
    );


    // --------------------------------------------------------
    // Start WiFi
    // --------------------------------------------------------

    WiFi.mode(
        WIFI_STA
    );

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );


    Serial.println(
        "Starting WiFi..."
    );
} // end of void setup


// ============================================================
// LOOP
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // WiFi
    // --------------------------------------------------------

    checkWiFi();


    // --------------------------------------------------------
    // MQTT
    // --------------------------------------------------------

    if (
        WiFi.status()
        == WL_CONNECTED
    )
    {
        if (!mqttClient.connected())
        {
            connectMQTT();
        }
        else
        {
            mqttClient.loop();
        }
    }


    // --------------------------------------------------------
    // Telemetry
    // --------------------------------------------------------

    unsigned long now =
        millis();


    if (
        mqttClient.connected()
        &&
        now - lastTelemetryTime
            >= TELEMETRY_INTERVAL
    )
    {
        lastTelemetryTime =
            now;

        publishTelemetry();
    }


    // --------------------------------------------------------
    // Small pause
    // --------------------------------------------------------

    delay(10);
} // end of void loop


// ==============================================================================================
// Start declaring functions --------------------------------------------------------------------

void connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    unsigned long now = millis();

    if (
        now - lastWiFiReconnectAttempt
        < WIFI_RECONNECT_INTERVAL
    )
    {
        return;
    }

    lastWiFiReconnectAttempt = now;

    Serial.println();
    Serial.println("Connecting to WiFi...");

    WiFi.mode(WIFI_STA);

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );
}

void checkWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        static bool wasConnected = false;

        if (!wasConnected)
        {
            Serial.println();
            Serial.println("WiFi connected.");

            Serial.print("ESP IP: ");
            Serial.println(
                WiFi.localIP()
            );

            wasConnected = true;
        }

        return;
    }

    Serial.println(
        "WiFi disconnected."
    );

    connectWiFi();
}

void publishStatus(const char* status)
{
    if (!mqttClient.connected())
    {
        return;
    }

    StaticJsonDocument<200> doc;

    doc["device"] = DEVICE_ID;
    doc["status"] = status;
    doc["firmware"] = FIRMWARE_VERSION;

    char buffer[200];

    serializeJson(
        doc,
        buffer,
        sizeof(buffer)
    );

    mqttClient.publish(
        TOPIC_STATUS,
        buffer,
        true
    );

    Serial.print(
        "Status published: "
    );

    Serial.println(buffer);
}

bool connectMQTT()
{
    if (
        WiFi.status()
        != WL_CONNECTED
    )
    {
        return false;
    }

    if (mqttClient.connected())
    {
        return true;
    }

    unsigned long now = millis();

    if (
        now - lastMQTTReconnectAttempt
        < MQTT_RECONNECT_INTERVAL
    )
    {
        return false;
    }

    lastMQTTReconnectAttempt = now;

    Serial.println();
    Serial.println(
        "Connecting to MQTT..."
    );

    // --------------------------------------------------------
    // Last Will message
    // --------------------------------------------------------

    StaticJsonDocument<200> offlineDoc;

    offlineDoc["device"] = DEVICE_ID;
    offlineDoc["status"] = "offline";
    offlineDoc["firmware"] = FIRMWARE_VERSION;

    char offlineBuffer[200];

    serializeJson(
        offlineDoc,
        offlineBuffer,
        sizeof(offlineBuffer)
    );


    String clientId =
        String(DEVICE_ID)
        + "-"
        + String(ESP.getChipId(), HEX);


    bool connected = mqttClient.connect(
        clientId.c_str(),

        TOPIC_STATUS,

        1,

        true,

        offlineBuffer
    );


    if (!connected)
    {
        Serial.print(
            "MQTT connection failed. State: "
        );

        Serial.println(
            mqttClient.state()
        );

        return false;
    }


    Serial.println(
        "MQTT connected."
    );


    // --------------------------------------------------------
    // Subscribe to commands
    // --------------------------------------------------------

    bool subscribed =
        mqttClient.subscribe(
            TOPIC_CONTROL,
            1
        );


    if (!subscribed)
    {
        Serial.println(
            "ERROR: Failed to subscribe to control topic."
        );

        return false;
    }


    Serial.println(
        "Subscribed to control topic."
    );


    // --------------------------------------------------------
    // Publish online status
    // --------------------------------------------------------

    publishStatus(
        "online"
    );


    return true;
}

void setLED(bool state)
{
    ledState = state;

    // ESP8266 built-in LED is active LOW.
    if (ledState)
    {
        digitalWrite(
            LED_BUILTIN,
            LOW
        );
    }
    else
    {
        digitalWrite(
            LED_BUILTIN,
            HIGH
        );
    }
}

void publishCommandResponse(const char* commandId, const char* command, bool success, const char* message)
{
    if (!mqttClient.connected())
    {
        return;
    }

    StaticJsonDocument<300> doc;

    doc["id"] = commandId;
    doc["device"] = DEVICE_ID;
    doc["command"] = command;
    doc["success"] = success;
    doc["message"] = message;

    char buffer[300];

    serializeJson(
        doc,
        buffer,
        sizeof(buffer)
    );

    mqttClient.publish(
        TOPIC_RESPONSE,
        buffer,
        false
    );

    Serial.print(
        "Command response: "
    );

    Serial.println(buffer);
}

void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.println();
    Serial.println(
        "MQTT message received."
    );

    Serial.print(
        "Topic: "
    );

    Serial.println(topic);


    // --------------------------------------------------------
    // Make payload into JSON
    // --------------------------------------------------------

    StaticJsonDocument<300> doc;

    DeserializationError error =
        deserializeJson(
            doc,
            payload,
            length
        );


    if (error)
    {
        Serial.print(
            "JSON error: "
        );

        Serial.println(
            error.c_str()
        );

        return;
    }


    // --------------------------------------------------------
    // Verify device
    // --------------------------------------------------------

    const char* device =
        doc["device"];

    if (device == nullptr)
    {
        Serial.println(
            "ERROR: Missing device."
        );

        return;
    }


    if (
        strcmp(
            device,
            DEVICE_ID
        ) != 0
    )
    {
        Serial.println(
            "ERROR: Command is for another device."
        );

        return;
    }


    // --------------------------------------------------------
    // Get command information
    // --------------------------------------------------------

    const char* commandId =
        doc["id"];

    const char* command =
        doc["command"];


    if (commandId == nullptr)
    {
        Serial.println(
            "ERROR: Missing command ID."
        );

        return;
    }


    if (command == nullptr)
    {
        Serial.println(
            "ERROR: Missing command."
        );

        return;
    }


    Serial.print(
        "Command ID: "
    );

    Serial.println(
        commandId
    );


    Serial.print(
        "Command: "
    );

    Serial.println(
        command
    );


    // --------------------------------------------------------
    // LED command
    // --------------------------------------------------------

    if (
        strcmp(
            command,
            "LED"
        ) == 0
    )
    {
        if (!doc["value"].is<bool>())
        {
            publishCommandResponse(
                commandId,
                command,
                false,
                "LED value must be boolean"
            );

            return;
        }


        bool value =
            doc["value"].as<bool>();


        setLED(
            value
        );


        publishCommandResponse(
            commandId,
            command,
            true,
            value
                ? "LED turned ON"
                : "LED turned OFF"
        );


        return;
    }


    // --------------------------------------------------------
    // Unknown command
    // --------------------------------------------------------

    publishCommandResponse(
        commandId,
        command,
        false,
        "Unknown command"
    );
}

void publishTelemetry()
{
    if (!mqttClient.connected())
    {
        return;
    }


    StaticJsonDocument<250> doc;

    doc["device"] = DEVICE_ID;

    // --------------------------------------------------------
    // Replace these with real sensors later.
    // Currently generating test values.
    // --------------------------------------------------------

    doc["temperature"] =
        random(20, 35);

    doc["humidity"] =
        random(40, 90);

    doc["light"] =
        random(0, 1023);


    doc["firmware"] =
        FIRMWARE_VERSION;


    char buffer[250];


    serializeJson(
        doc,
        buffer,
        sizeof(buffer)
    );


    bool result =
        mqttClient.publish(
            TOPIC_DATA,
            buffer,
            false
        );


    if (result)
    {
        Serial.print(
            "Telemetry: "
        );

        Serial.println(
            buffer
        );
    }
    else
    {
        Serial.println(
            "ERROR: Failed to publish telemetry."
        );
    }
}