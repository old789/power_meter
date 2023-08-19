
#define PZEM004_NO_SWSERIAL

//#define WIFI_ENABLE

//#include <ModbusMaster.h>  // not use with PZEM004Tv30
#include <PZEM004Tv30.h>

#ifdef WIFI_ENABLE
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#endif

#ifndef PZEM004_NO_SWSERIAL
#include <SoftwareSerial.h> // ( NODEMCU ESP8266 )
#endif

/*
/SoftwareSerial pzem(D5,D6); // (RX,TX) connect to TX,RX of PZEM for NodeMCU
#include <ModbusMaster.h>
ModbusMaster node;
*/

#include <U8x8lib.h>

#define SCL_PIN SCL  // SCL pin of OLED. Default: D1 (ESP8266) or D22 (ESP32)
#define SDA_PIN SDA  // SDA pin of OLED. Default: D2 (ESP8266) or D21 (ESP32)
#define MEASUREMENTS 10
#define MAIN_DELAY 1000

#ifdef PZEM004_NO_SWSERIAL

#define PZEM_SERIAL Serial
#define CONSOLE Serial1

PZEM004Tv30 pzem(PZEM_SERIAL);

#else

#define PZEM_RX_PIN 14
#define PZEM_TX_PIN 12
#define CONSOLE Serial

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSWSerial);

#endif

// U8X8 Display constructors: https://github.com/olikraus/u8g2/wiki/u8x8setupcpp
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(SCL_PIN, SDA_PIN, U8X8_PIN_NONE);

#ifdef WIFI_ENABLE
//WiFi data
char ssid[] = "SSID"; //WiFi Credential
char pass[] = "PASSW"; //WiFi Password

//Domain name with URL
const char* serverName = "http://10.10.10.10/pwr/pwrm.php";

WiFiClient client;

#endif

int cnt=0;
int i=0;
double U_PR, I_PR, P_PR, PPR, PR_F, PR_PF;
unsigned long upcounter=0;
bool rc=false;

char str_voltage[8];
char str_current[8];
char str_power[16];
char str_energy[16];
char str_freq[8];
char str_pfactor[8];
char str_tmp[64];
char str_post[4096];

double main_buffer [MEASUREMENTS+1][6];

void setup(){
#ifdef PZEM004_NO_SWSERIAL
  Serial.swap();
#endif
  CONSOLE.begin(115200);
  delay(50);
  CONSOLE.println("Start serial");

  // initialize OLED
  u8x8.begin();
  // u8x8.setBusClock(400000);
  u8x8.setFont(u8x8_font_8x13_1x2_f);
  u8x8.drawString(1, 0, "Booting...");

/*
  pzem.begin(9600);
  CONSOLE.println("Start PZEM serial");
  node.begin(1, pzem);  // 1 = ID MODBUS
  CONSOLE.println("Start PZEM");
*/  

  // clear PZEM energy counter
  PPR = pzem.energy();
  if ( ( !isnan(PPR) ) && ( PPR > 0 ) ) {
    if ( pzem.resetEnergy()) {
      CONSOLE.println("Reset energy counter");
    }else{
      CONSOLE.println("Can't reset energy counter");
    }
  }
 
  #ifdef WIFI_ENABLE

  CONSOLE.print("Connecting to ");
  CONSOLE.print(ssid);
  CONSOLE.println(" ...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);             // Connect to the network

  CONSOLE.println("Is connect?..");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    CONSOLE.print(++i); CONSOLE.print(' ');
  }

  CONSOLE.println('\n');
  CONSOLE.println("Connection established!");  
  CONSOLE.print("IP address:\t");
  CONSOLE.println(WiFi.localIP());
    
#endif
}

void loop(){

  CONSOLE.println("Read PZEM");  

/*
  result = node.readInputRegisters(0x0000, 10);
  if (result == node.ku8MBSuccess) {
    U_PR = (node.getResponseBuffer(0x00)/10.0f);
    I_PR = (node.getResponseBuffer(0x01)/1000.000f);
    P_PR = (node.getResponseBuffer(0x03)/10.0f);
    PPR = (node.getResponseBuffer(0x05)/1000.0f);
    PR_F = (node.getResponseBuffer(0x07)/10.0f);
    PR_PF = (node.getResponseBuffer(0x08)/100.0f);
  }
*/

  //CONSOLE.print("Read PZEM,custom address:");
  //CONSOLE.println(pzem.readAddress(), HEX);

  CONSOLE.print("UpCounter="); CONSOLE.println(upcounter++);
  CONSOLE.print("Counter="); CONSOLE.println(cnt);

  u8x8.drawString(0, 0, "  Measurement");

  rc=true;
  // Read the data from the sensor
  U_PR = pzem.voltage();
  I_PR = pzem.current();
  P_PR = pzem.power();
  PPR = pzem.energy();
  PR_F = pzem.frequency();
  PR_PF = pzem.pf();

  // Check if the data is valid
  if(isnan(U_PR)){
    CONSOLE.println("Error reading voltage");
    U_PR=0;
    rc=false;
  } else if (isnan(I_PR)) {
    CONSOLE.println("Error reading current");
    I_PR=0;        
    rc=false;
  } else if (isnan(P_PR)) {
    CONSOLE.println("Error reading power");
    P_PR=0;
    rc=false;
  } else if (isnan(PPR)) {
    CONSOLE.println("Error reading energy");
    PPR=0;
    rc=false;
  } else if (isnan(PR_F)) {
    CONSOLE.println("Error reading frequency");
    PR_F=0;
    rc=false;
  } else if (isnan(PR_PF)) {
    CONSOLE.println("Error reading power factor");
    rc=false;
    PR_PF=0;
  }

  if (! rc){
    u8x8.clearDisplay();
    u8x8.setCursor(5,2);
    u8x8.print("Error!");
    delay(MAIN_DELAY);
    return;
  }

  main_buffer[cnt][0]=U_PR;
  main_buffer[cnt][1]=I_PR;
  main_buffer[cnt][2]=P_PR;
  main_buffer[cnt][3]=PPR;
  main_buffer[cnt][4]=PR_F;
  main_buffer[cnt][5]=PR_PF;
        
  //dtostrf(U_PR,1,1,str_voltage);
  //dtostrf(I_PR,1,3,str_current);
  //dtostrf(P_PR,1,1,str_power);
  //dtostrf(PPR,1,1,str_energy);
  //dtostrf(PR_F,1,1,str_freq);
  //dtostrf(PR_PF,1,1,str_pfactor);
  //sprintf(str_tmp,"%s,%s,%s,%s,%s,%s",str_voltage,str_current,str_power,str_energy,str_freq,str_pfactor);
   
  CONSOLE.print("U_PR: "); CONSOLE.println(U_PR); // V
  CONSOLE.print("I_PR: "); CONSOLE.println(I_PR,3); // A
  CONSOLE.print("P_PR: "); CONSOLE.println(P_PR); // W
  CONSOLE.print("PPR: "); CONSOLE.println(PPR,3); // kWh
  CONSOLE.print("PR_F: "); CONSOLE.println(PR_F); // Hz
  CONSOLE.print("PR_PF: "); CONSOLE.println(PR_PF);

  //CONSOLE.println(str_tmp);
  
  u8x8.setCursor(1,2);
  u8x8.print(P_PR,2);
  u8x8.print("W (");
  u8x8.print(PR_PF,2);
  u8x8.print(")");
  u8x8.setCursor(1,4);
  u8x8.print(U_PR,1);
  u8x8.print("V ");
  u8x8.print(I_PR,3);
  u8x8.print("A");
  u8x8.setCursor(1,6);
  u8x8.print(PR_F,1);
  u8x8.print("Hz ");
  u8x8.print(PPR,2);
  //u8x8.print("");
  
  cnt++;
  if (cnt == MEASUREMENTS) {
    cnt=0;     
#ifdef WIFI_ENABLE
    CONSOLE.println("Send data");
    //Check WiFi connection status
    if(WiFi.status() != WL_CONNECTED){
      CONSOLE.println("WiFi Disconnected");
    }else{
      //CONSOLE.println("HTTP client");
      HTTPClient http;

      memset(str_post,0,sizeof(str_post));    
    
      for( i=0; i<MEASUREMENTS; i++ ){
        dtostrf(main_buffer[i][0],1,1,str_voltage);
        dtostrf(main_buffer[i][1],1,3,str_current);
        dtostrf(main_buffer[i][2],1,1,str_power);
        dtostrf(main_buffer[i][3],1,1,str_energy);
        dtostrf(main_buffer[i][4],1,1,str_freq);
        dtostrf(main_buffer[i][5],1,1,str_pfactor);
        sprintf(str_tmp,"m%u=%s,%s,%s,%s,%s,%s&",i,str_voltage,str_current,str_power,str_energy,str_freq,str_pfactor);
        if (i > 0){
          strncat(str_post,str_tmp,sizeof(str_post)-1);
        }else{
          strncpy(str_post,str_tmp,sizeof(str_post)-1);
        }
      }

      //CONSOLE.println("http begin");
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
  
      // If you need server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
    
      //CONSOLE.println("http header");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");    
      //http.addHeader("Content-Type", "text/plain");
      //CONSOLE.println("http post");
      int httpResponseCode = http.POST(str_post);
     
      //CONSOLE.print("HTTP Response code: ");
      //CONSOLE.println(httpResponseCode);
      CONSOLE.println("Free resources");
  
      // Free resources
      http.end();
    }
#endif
  }
  //CONSOLE.println("End of loop, sleeping...");
  delay(MAIN_DELAY);
}
