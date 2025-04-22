#include <Arduino.h>
#include <mcp_can.h>
#include <SPI.h>
#include <LiquidCrystal.h>

const int SPI_CS_pin = 3;
const int valvePin = 2; // Valve control pin
MCP_CAN CAN(SPI_CS_pin);
LiquidCrystal lcd(7, 8, 9, 10, 11, 12); 

void requestData(byte len, byte mode, byte pid) {
  byte RequestdataArr[] = {len, mode, pid, 0x00, 0x00, 0x00, 0x00, 0x00};
  CAN.sendMsgBuf(0x7DF, 0, sizeof(RequestdataArr), RequestdataArr);
}

double processData(byte mode, byte pid, byte rxBuf[]) {
  double value = 0;
  switch (pid) {
    case 0x0C: 
      value = ((rxBuf[3] * 256) + rxBuf[4]) / 4;
      break;
  }
  return value;
}

double readData(byte len, byte mode, byte pid) {
  long unsigned int rxID;
  unsigned char rxLen = 0;
  unsigned char rxBuf[8];
  unsigned long startTime = millis();

  requestData(len, mode, pid);

  while (millis() - startTime < 200) {
    if (CAN.checkReceive() == CAN_MSGAVAIL) {
      CAN.readMsgBuf(&rxID, &rxLen, rxBuf);
      if (rxID == 0x7E8 && rxBuf[1] == (mode + 0x40) && rxBuf[2] == pid) {
        double value = processData(mode, pid, rxBuf);
        return value;
      }
    }
  }
  return -1; // Failed to get a valid response
}

void controlValve(double rpm) {
  if (rpm > 4000) {
    digitalWrite(valvePin, HIGH); 
    Serial.println("Valve OPEN");
  } else if (rpm < 3500) {
    digitalWrite(valvePin, LOW);
    Serial.println("Valve CLOSED");
  }
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(valvePin, OUTPUT);

  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("CAN BUS initiated");
  } else {
    Serial.println("CAN BUS Failed");
    while (1);
  }
}

void loop() {
  double rpm = readData(8, 0x01, 0x0C);

  if (rpm >= 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RPM:");
    lcd.setCursor(0, 1);
    lcd.print(rpm);
    controlValve(rpm);
  } else {
    Serial.println("RPM read failed.");
  }

  delay(500);
}
