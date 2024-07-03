#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>


const char* ssid = "Airtel_6767";
const char* password = "";
const char* aws_endpoint = "";

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* device_cert = R"EOF(
-----BEGIN CERTIFICATE-----

)EOF";


const char* private_key = R"EOF(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)EOF";


const char* aws_root_ca = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";


const char* topic_on = "esp8266/control/on";
const char* topic_off = "esp8266/control/off";
const char* topic_temp_humid = "esp8266/temperature_humidity";
const char* topic_request_temp_humid = "esp8266/request/temperature_humidity";
 
BearSSL::WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println(" Connected!");

  
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println(" done!");

   
    BearSSL::X509List cert(device_cert);
    BearSSL::PrivateKey key(private_key);
    BearSSL::X509List ca(aws_root_ca);

    espClient.setClientRSACert(&cert, &key);
    espClient.setTrustAnchors(&ca);
    espClient.setTimeout(10000);

    client.setServer(aws_endpoint, 8883);
    client.setCallback(callback);

    Serial.print("Resolving AWS IoT endpoint...");
    IPAddress ip;
    if (WiFi.hostByName(aws_endpoint, ip)) {
        Serial.print("AWS IoT IP: ");
        Serial.println(ip);
    } else {
        Serial.println("Failed to resolve endpoint.");
    }

    connectToMQTT();
    dht.begin();
}

void loop() {
    if (!client.connected()) {
        connectToMQTT();
    }
    client.loop();
}

void connectToMQTT() {
    Serial.print("Connecting to MQTT broker...");
    while (!client.connected()) {
        if (client.connect("ESP8266_Client")) {
            Serial.println("Connected!");
            client.subscribe(topic_on);
            client.subscribe(topic_off);
            client.subscribe(topic_request_temp_humid);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Trying again in 5 seconds...");
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (String(topic) == topic_on) {
        digitalWrite(LED_BUILTIN, LOW); 
        Serial.println("Bulb is ON");
    } else if (String(topic) == topic_off) {
        digitalWrite(LED_BUILTIN, HIGH); 
        Serial.println("Bulb is OFF");
    } else if (String(topic) == topic_request_temp_humid) {
      
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();
        Serial.println(temperature,humidity,"meow");
        String payload = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
        client.publish(topic_temp_humid, payload.c_str());
    }
}
