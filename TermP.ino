#include "config.h" // Wifi and IFTTT
#include <WiFi.h>
#include <WiFiClient.h>
#include <M5Stack.h>
#include <SoftwareSerial.h>
SoftwareSerial s_serial(17, 16);

#define sensor s_serial

bool outputState = true;
unsigned long t = 0; // Time


const char* server = "maker.ifttt.com";  // Server URL

WiFiClient client;

unsigned long duration;
unsigned long starttime;
unsigned long times = 60000;  //30秒;
unsigned long lowpulseoccupancy = 0;
float rate = 0;
float concentration = 0;

const unsigned char cmd_get_sensor[] =
{
    0xff, 0x01, 0x86, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x79
};

unsigned char dataRevice[9];
int temperature;
int CO2PPM;
 

void setup() {
    M5.begin();
    pinMode(17,OUTPUT);
    pinMode(16,INPUT);
    pinMode(2, INPUT);
    M5.Lcd.setTextSize(3);
    sensor.begin(9600);
    Serial.begin(115200);
    Serial.println("get a 'g', begin to read from sensor!");
    Serial.println("********************************************************");
    Serial.println();
    starttime = millis();//現在の時間を測る;
                 
  //Initialize serial and wait for port to open:

  wifiBegin();
  while (!checkWifiConnected()) {
    wifiBegin();
  }
}

void loop() {

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Dust: ");
  M5.Lcd.println(readDust());
  if(readDust()>5000 || CO2PPM>1500){
  send();
  }
  if(dataRecieve())
    {
        M5.Lcd.print("Temperature: ");
        M5.Lcd.println(temperature);
        M5.Lcd.print("CO2: ");
        M5.Lcd.print(CO2PPM);
        M5.Lcd.println("");
    }
    delay(100);

  
}

void wifiBegin() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
}

bool checkWifiConnected() {
  // attempt to connect to Wifi network:
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);

    count++;
    if (count > 15) { // about 15s
      Serial.println("(wifiConnect) failed!");
      return false;
    }
  }

  Serial.print("Connected to ");
  Serial.println(ssid);

  return true;
}

void send() {
  while (!checkWifiConnected()) {
    wifiBegin();
  }

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, 80)) {
    Serial.println("Connection failed!");
  } else {
    Serial.println("Connected to server!");
    // Make a HTTP request:
    String url = "/trigger/" + makerEvent + "/with/key/" + makerKey;
   // url += "?value1=" + value1;
    client.println("GET " + url + " HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();

    Serial.print("Waiting for response "); //WiFiClientSecure uses a non blocking implementation

    int count = 0;
    while (!client.available()) {
      delay(10000); //
      Serial.print(".");

      count++;
      if (count > 20 * 20) { // about 20s
        Serial.println("(send) failed!");
        return;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting from server.");
      client.stop();
    }
  }
}

float readDust(){
      //ハウスダスト
  duration = pulseIn(2, LOW);  // パルスがLowである時間を測る
    lowpulseoccupancy += duration; // Lowである時間を積算する
    if ((millis()-starttime) > times){
        rate = lowpulseoccupancy/(times*10.0);  // 積算時間 / 30秒の比率の100倍を計算する
        concentration = 1.1*pow(rate,3)-3.8*pow(rate,2)+520*rate+0.62; // 比率から粒子量を計算する
        Serial.println(concentration);
      
        //M5.Lcd.setCursor(10, 60);
        //M5.Lcd.print(concentration);
      
        lowpulseoccupancy = 0;
        starttime = millis();
   }
  return float(concentration);
}

bool dataRecieve(void)
{
    byte data[9];
    int i = 0;
 
    //transmit command data
    for(i=0; i<sizeof(cmd_get_sensor); i++)
    {
        sensor.write(cmd_get_sensor[i]);
    }
    delay(10);
    //begin reveiceing data
    if(sensor.available())
    {
        while(sensor.available())
        {
            for(int i=0;i<9; i++)
            {
                data[i] = sensor.read();
            }
        }
    }
 
    for(int j=0; j<9; j++)
    {
        Serial.print(data[j]);
        Serial.print(" ");
    }
    Serial.println("");
 
    if((i != 9) || (1 + (0xFF ^ (byte)(data[1] + data[2] + data[3] + data[4] + data[5] + data[6] + data[7]))) != data[8])
    {
        return false;
    }
 
    CO2PPM = (int)data[2] * 256 + (int)data[3];
    temperature = (int)data[4] - 40;
 
    return true;
}
