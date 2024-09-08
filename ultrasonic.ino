#include <WiFiNINA.h>
#include <PubSubClient.h>

// WiFi and MQTT credentials
const char* ssid = "Jio2199";  // Replace with your Wi-Fi network
const char* password = "1122334455";  // Replace with your Wi-Fi password
const char* mqtt_server = "s6820619.ala.dedicated.aws.emqxcloud.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "SIT210-WAVE";
const char* mqtt_username = "samarth";  // Replace with your MQTT username
const char* mqtt_password = "123456789";  // Replace with your MQTT password

// Ultrasonic sensor pins
const int trigPin = 7;
const int echoPin = 6;
long duration;
int distance;

// LED pin
const int ledPin = 13;
unsigned long patStartTime = 0;
bool patActive = false;
const unsigned long patDuration = 6000;  // 6 seconds for the LED to stay on

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Function to connect to WiFi
void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

// Callback function when a message is received from the MQTT topic
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived: ");
  Serial.println(message);

  // Handle the wave message: blink LED 3 times
  if (message == "wave") {
    Serial.println("Handling wave...");
    for (int i = 0; i < 3; i++) {
      digitalWrite(ledPin, HIGH);
      delay(500);
      digitalWrite(ledPin, LOW);
      delay(500);
    }
  }
  // Handle the pat message: Activate the LED for 6 seconds
  else if (message == "pat") {
    Serial.println("Handling pat...");
    patStartTime = millis();  // Record the start time
    patActive = true;         // Activate the pat behavior
    digitalWrite(ledPin, HIGH);  // Turn the LED on immediately
  }
}

// Function to connect to the MQTT broker with username and password
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Connect with username and password
    if (client.connect("ArduinoClient", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(mqtt_topic);
      Serial.println("Subscribed to topic: SIT210-WAVE");
    } else {
      Serial.print("Failed to connect, return code=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);  // Wait 5 seconds before retrying
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Set up ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Set up LED pin
  pinMode(ledPin, OUTPUT);
  
  // Initialize WiFi and MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  client.setKeepAlive(60);  // Set keep-alive interval for MQTT
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Handle the "pat" non-blocking LED behavior
  if (patActive && millis() - patStartTime >= patDuration) {
    digitalWrite(ledPin, LOW);  // Turn the LED off after 6 seconds
    patActive = false;          // Reset the flag
  }

  // Ultrasonic sensor: trigger the wave detection
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echo signal
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  // Print distance to monitor for debugging
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // If the distance is less than a threshold (e.g., 10 cm), publish a message
  if (distance < 10) {
    Serial.println("Wave detected!");
    client.publish(mqtt_topic, "wave");
    delay(1000);  // Avoid spamming the broker
  }
}
