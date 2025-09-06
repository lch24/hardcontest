void sendCommand(String cmd) {
  Serial.print(cmd);
  Serial.print("\r\n"); 
  delay(20 + cmd.length() * 2); 
}

void setup() {
  Serial.begin(115200);
  delay(2000); 
  sendCommand("CLS(0);"); 
  delay(100);
  sendCommand("SPG(34);");
  delay(1000); 
}

void loop() {
  for(int i = 26; i <= 33; i++) {
    String cmd = "SPG(" + String(i) + ");";
    sendCommand(cmd);
    delay(300); 
  }
}