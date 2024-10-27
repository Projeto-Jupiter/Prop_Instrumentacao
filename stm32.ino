#include <HardwareSerial.h>

// Define the pins used for E32 LoRa module
#define AUX_PIN PA3  // AUX pin from E32 (example GPIO pin PA3)

void setup() {
  // Start the serial communication for monitoring (connected to USB)
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for the Serial port to connect
  }

  // Start the hardware serial for data transmission (connected to PA9 TX, PA10 RX)
  Serial1.begin(9600);  // Use USART1 (connected to PA9 TX, PA10 RX)

  // Set the AUX pin mode
  pinMode(AUX_PIN, INPUT);

  Serial.println("Data Rate Test Initialized on STM32");
}

void loop() {
  unsigned long startTime = millis();
  unsigned long bytesSent = 0;
  unsigned long duration = 60000;  // 5 seconds to test data rate
  unsigned int packetSize = 512;  // Define the packet size in bytes (256 bytes per packet)
  unsigned long targetBitsPerSecond = 2048;  // Target is 2.048 kbps (2 KiB per second)

  // Keep sending packets for a fixed duration, respecting the target rate
  while (millis() - startTime < duration) {
    // Create a packet of size 256 bytes
    String message = "";
    for (int i = 0; i < packetSize; i++) {
      message += "A";  // Fill the packet with repetitive characters
    }
    message += "\n";

    unsigned long packetStartTime = millis();
    Serial1.print(message);
    bytesSent += message.length();

    // Ensure we meet the target rate of 2 KiB per second
  }

  // Calculate data sent in the given duration
  float kilobytesSent = bytesSent / 1024.0;
  float dataRateKbps = (kilobytesSent * 8) / (duration / 1000.0);  // Convert to kilobits per second

  Serial.print("Data Sent: ");
  Serial.print(kilobytesSent, 2);
  Serial.println(" KB");

  Serial.print("Data Rate: ");
  Serial.print(dataRateKbps, 2);
  Serial.println(" kbps");

  // Optional delay to avoid repeating the test too quickly
  delay(2000);
}
