#include <Wire.h>
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
SoftwareSerial WifiSerial(10, 11); // Pin 17 and 18 act as RX and TX
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DEBUG true

String mySSID = "";     // WiFi Name/SSID
String myPWD = "";      // WiFi Password
String serverIP = "";   // Python Server Local IP
String serverPort = ""; // Python Server Port
String pingIP = "1.1.1.1";
String pingIP2 = "8.8.8.8";
String pingIP3 = "8.8.4.4";

int RouterOFFDelay = 5000;
int PingAgainTimer = 20;
String Str_PingAgainTimer = String(PingAgainTimer);

String formattedMessage;
String message;
String downloadSpeedResult;
String uploadSpeedResult;
String pingResult;

int RELAY_Pin = 12;
int BUTTON_Pin_Speed = 2;
int BUTTON_Pin_Reset = 3;
int buttonread_Speed;
int buttonread_Reset;

boolean wasSpeedButtonPressed = false;
boolean wasResetButtonPressed = false;

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(1000);
  WifiSerial.begin(115200);
  WifiSerial.setTimeout(1000);

  Timer1.initialize(500000);
  Timer1.attachInterrupt(PressedButton);

  pinMode(RELAY_Pin, OUTPUT);
  pinMode(BUTTON_Pin_Speed, INPUT);
  pinMode(BUTTON_Pin_Reset, INPUT);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();

  InitWifiModule();
}

void loop()
{
  PingServer(True);
}

String espData(String command, const int timeout, boolean debug)
{
  Serial.print("AT Command ==> ");
  Serial.print(command);
  Serial.println("     ");

  String response = "";
  WifiSerial.println(command);
  long int time = millis();
  while ((time + timeout) > millis())
  {
    while (WifiSerial.available())
    {
      char c = WifiSerial.read();
      response += c;
    }
  }
  if (debug)
  {
    Serial.println(response);
  }
  return response;
}

String onlyGetString(String receivedMessage)
{
  int messageLength = 0, delimiter = 0;
  String lmsg;
  messageLength = receivedMessage.length();
  for (int i = 0; i < messageLength; i++)
  {
    if (receivedMessage[i] == ':')
    {
      delimiter = i;
    }
    if (i > delimiter && delimiter != 0)
    {
      lmsg += receivedMessage[i];
    }
  }
  formattedMessage = "";
  return lmsg;
}

void ShowLCD(String line1, int column1, String line2, int column2, boolean isClear, int timeout)
{
  if (isClear)
  {
    lcd.clear();
  }
  lcd.setCursor(column1, 0);
  lcd.print(line1);
  lcd.setCursor(column2, 1);
  lcd.print(line2);
  delay(timeout);

  CheckButton();
}

void InitWifiModule()
{
  ShowLCD("Setting Up", 3, "", 0, true, 0);
  espData("AT+RST", 2000, false);
  digitalWrite(RELAY_Pin, HIGH); // Turn on Router
  delay(2000);
  espData("AT+CWQAP", 2000, DEBUG); // Disconnect All Saved Connections
  espData("AT+CWMODE=1", 2000, DEBUG);
  ShowLCD("", 0, "OK", 7, false, 5000);

  ShowLCD("Connecting Wifi", 0, "", 0, true, 0);
  String wifiConnecting = espData("AT+CWJAP=\"" + mySSID + "\",\"" + myPWD + "\"", 8000, DEBUG); // Connect to WiFi network
  while (!(wifiConnecting.indexOf("OK") > 0))
  { // Retry Connecting to WiFi network
    wifiConnecting = espData("AT+CWJAP=\"" + mySSID + "\",\"" + myPWD + "\"", 8000, DEBUG);
  }
  ShowLCD("", 0, "OK", 7, false, 5000);
}

void RestartWifiModule()
{
  wasResetButtonPressed = false;
  digitalWrite(RELAY_Pin, LOW); // Turn off Router
  ShowLCD("Restarting", 2, "Router Now", 2, true, RouterOFFDelay);
  InitWifiModule();
}

void GetWifiSpeed()
{
  wasSpeedButtonPressed = false;
  ShowLCD("Getting Wifi", 2, "Speed", 5, true, 0);

  String connectTCP = espData("AT+CIPSTART=\"TCP\",\"" + serverIP + "\"," + serverPort, 5000, DEBUG);
  if (connectTCP.indexOf("ERROR") > 0 || connectTCP.indexOf("CLOSED") > 0)
  {
    Serial.println("Failed Connection from Python Server");
    ShowLCD("Fail Connection", 0, "Python Server", 1, true, 5000);
    return;
  }
  String sendData = espData("AT+CIPSEND=7", 5000, DEBUG);
  if (sendData.indexOf("ERROR") > 0)
  {
    Serial.println("Failed Connection from Python Server");
    ShowLCD("Fail Connection", 0, "Python Server", 1, true, 5000);
    return;
  }
  WifiSerial.println("Execute");

  espData("AT+CIPCLOSE", 2000, false);

  String message;
  int counter = 40;

  while ((WifiSerial.available() == 0) || (WifiSerial.read() == ""))
  {
    counter--;
    delay(1000);
    if (counter == 0)
    {
      break;
    }
  }
  delay(5000);
  formattedMessage = WifiSerial.readStringUntil('\r');
  message = onlyGetString(formattedMessage);
  Serial.println("Received from Server: " + message);

  int i = 0;
  while (message.charAt(i) != 'b')
  {
    if (i == 25)
    {
      break;
    }
    i++;
  }

  if ((message != "") && (i == 25))
  {

    i = 0;
    while (message.charAt(i) != '<')
    {
      if (i == 50)
      {
        Serial.println("Data Received Error");
        return;
      }
      i++;
    }
    downloadSpeedResult = message.substring(i + 1, i + 4);
    ShowLCD("Download Speed:", 0, downloadSpeedResult + " Mb/s", 4, true, 5000);

    i = 0;
    while (message.charAt(i) != '>')
    {
      if (i == 50)
      {
        Serial.println("Data Received Error");
        return;
      }
      i++;
    }
    uploadSpeedResult = message.substring(i + 1, i + 4);
    ShowLCD("Upload Speed:", 1, uploadSpeedResult + " Mb/s", 4, true, 5000);

    i = 0;
    while (message.charAt(i) != '~')
    {
      if (i == 50)
      {
        Serial.println("Data Received Error");
        return;
      }
      i++;
    }
    pingResult = message.substring(i + 1, i + 5);
    ShowLCD("Latency:", 4, pingResult + " ms", 5, true, 5000);
  }
  else
  {
    Serial.println("Failed Connection from Python Server");
    ShowLCD("Fail Connection", 0, "Python Server", 1, true, 5000);
  }
}

void PingServer(boolean checkWifi)
{
  ShowLCD("Pinging Servers:", 0, "", 0, true, 3000);
  ShowLCD("Server: " + pingIP, 0, "", 0, true, 0);
  String pingResult = espData("AT+PING=\"" + pingIP + "\"", 6000, DEBUG);
  if (!(pingResult.indexOf("OK") > 0))
  {
    ShowLCD("", 0, "FAIL", 6, false, 3000);
    ShowLCD("Server: " + pingIP2, 0, "", 0, true, 0);
    pingResult = espData("AT+PING=\"" + pingIP2 + "\"", 6000, DEBUG);
    if (!(pingResult.indexOf("OK") > 0))
    {
      ShowLCD("", 0, "FAIL", 6, false, 3000);
      ShowLCD("Server: " + pingIP3, 0, "", 0, true, 0);
      pingResult = espData("AT+PING=\"" + pingIP3 + "\"", 6000, DEBUG);
    }
  }
  if (!(pingResult.indexOf("OK") > 0))
  {
    ShowLCD("", 0, "FAIL", 6, false, 3000);
    ShowLCD("Server No Reply", 0, "", 0, true, 0);
    ShowLCD("", 0, "Restart Router", 1, false, 0);
    RestartWifiModule();
  }
  else
  {
    ShowLCD("", 0, "OK", 7, false, 3000);
    if (checkWifi)
    {
      GetWifiSpeed();
    }
    ShowLCD("Pinging Again", 1, "After " + Str_PingAgainTimer + " Seconds", 0, true, 3000);
    for (int i = PingAgainTimer; i >= 0; i--)
    {
      String time = String(i);
      ShowLCD("Timer:", 5, time + " second/s", 3, true, 1000);
      Serial.println("Pinging Again after: " + time + " sec/s");
    }
  }
}

void PressedButton()
{
  buttonread_Reset = digitalRead(BUTTON_Pin_Reset);

  buttonread_Speed = digitalRead(BUTTON_Pin_Speed);
  if (buttonread_Speed)
  {
    wasSpeedButtonPressed = true;
  }
  if (buttonread_Reset)
  {
    wasResetButtonPressed = true;
  }
}

void CheckButton()
{
  if (wasResetButtonPressed)
  {
    RestartWifiModule();
  }
  if (wasSpeedButtonPressed)
  {
    GetWifiSpeed();
  }
}
