//#include <ModbusMaster.h>  // not use with PZEM004Tv30
#include <ESP8266WiFi.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h> // ( NODEMCU ESP8266 )
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

/*
/SoftwareSerial pzem(D5,D6); // (RX,TX) connect to TX,RX of PZEM for NodeMCU
#include <ModbusMaster.h>
ModbusMaster node;
*/

#define MEASUREMENTS 10

#define PZEM_RX_PIN 14
#define PZEM_TX_PIN 12

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSWSerial);

//WiFi data
char ssid[] = "SSID"; //WiFi Credential
char pass[] = "PASSW"; //WiFi Password

//Domain name with URL
const char* serverName = "http://10.10.10.10/pwr/pwrm.php";

int cnt=0;
int i=0;
double U_PR, I_PR, P_PR, PPR, PR_F, PR_PF;
//uint8_t result;
//uint16_t data[6];

WiFiClient client;

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
  Serial.begin(115200);
  delay(50);
  Serial.println("Start serial");
/*
  pzem.begin(9600);
  Serial.println("Start PZEM serial");
  node.begin(1, pzem);  // 1 = ID MODBUS
  Serial.println("Start PZEM");
*/  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
    
}

void loop(){
/*
  Serial.println("Read PZEM");  

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

  Serial.print("Read PZEM,custom address:");
  Serial.println(pzem.readAddress(), HEX);

  Serial.print("Counter="); Serial.println(cnt);

  // Read the data from the sensor
  U_PR = pzem.voltage();
  I_PR = pzem.current();
  P_PR = pzem.power();
  PPR = pzem.energy();
  PR_F = pzem.frequency();
  PR_PF = pzem.pf();

  // Check if the data is valid
  if(isnan(U_PR)){
    Serial.println("Error reading voltage");
    U_PR=0;        
  } else if (isnan(I_PR)) {
    Serial.println("Error reading current");
    I_PR=0;        
  } else if (isnan(P_PR)) {
    Serial.println("Error reading power");
    P_PR=0;
  } else if (isnan(PPR)) {
    Serial.println("Error reading energy");
    PPR=0;
  } else if (isnan(PR_F)) {
    Serial.println("Error reading frequency");
    PR_F=0;
  } else if (isnan(PR_PF)) {
    Serial.println("Error reading power factor");
    PR_PF=0;
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
   
  //Serial.print("U_PR: "); Serial.println(U_PR); // V
  //Serial.print("I_PR: "); Serial.println(I_PR,3); // A
  //Serial.print("P_PR: "); Serial.println(P_PR); // W
  //Serial.print("PPR: "); Serial.println(PPR,3); // kWh
  //Serial.print("PR_F: "); Serial.println(PR_F); // Hz
  //Serial.print("PR_PF: "); Serial.println(PR_PF);

  //Serial.println(str_tmp);

  cnt++;
  if (cnt == MEASUREMENTS) {
    Serial.println("Send data");
    cnt=0;     
    //Check WiFi connection status
    if(WiFi.status() != WL_CONNECTED){
      Serial.println("WiFi Disconnected");
    }else{
      //Serial.println("HTTP client");
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

      //Serial.println("http begin");
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
  
      // If you need server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
    
      //Serial.println("http header");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");    
      //http.addHeader("Content-Type", "text/plain");
      //Serial.println("http post");
      int httpResponseCode = http.POST(str_post);
     
      //Serial.print("HTTP Response code: ");
      //Serial.println(httpResponseCode);
      Serial.println("Free resources");
  
      // Free resources
      http.end();
    }
  }
  //Serial.println("End of loop, sleeping...");
  delay(1000);
}
