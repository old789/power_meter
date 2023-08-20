
#define PZEM004_NO_SWSERIAL

//#define WIFI_ENABLE

#include <PZEM004Tv30.h>  // https://github.com/mandulaj/PZEM-004T-v30

#ifdef WIFI_ENABLE
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#endif

#ifndef PZEM004_NO_SWSERIAL
#include <SoftwareSerial.h>
#endif

#include <U8x8lib.h>    // https://github.com/olikraus/u8g2

#define SCL_PIN SCL  // SCL pin of OLED. Default: D1 (ESP8266) or D22 (ESP32)
#define SDA_PIN SDA  // SDA pin of OLED. Default: D2 (ESP8266) or D21 (ESP32)
#define LCD_COLS 16
#define LCD_ROWS 4
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

double voltage, current, power, energy, freq, pwfactor;
unsigned long upcounter=0;
bool rc=false;
uint8_t roll_cnt=0;
char roller[] = { '-', '/', '|', '\\' };

char str_voltage[8];
char str_current[8];
char str_power[16];
char str_energy[16];
char str_freq[8];
char str_pfactor[8];
char str_tmp[64];

char screen_cur[LCD_ROWS][LCD_COLS+1];
char screen_prev[LCD_ROWS][LCD_COLS+1];

#ifdef WIFI_ENABLE
int cnt=0;
double main_buffer [MEASUREMENTS+1][6];
char str_post[4096];
#endif

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
  memset(screen_prev,0,sizeof(screen_prev));
  
  // clear PZEM energy counter
  energy = pzem.energy();
  if ( ( !isnan(energy) ) && ( energy > 0 ) ) {
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

  CONSOLE.println("\n");  
  CONSOLE.print("Round "); CONSOLE.println(upcounter++);
  CONSOLE.println("Read PZEM");  

  //CONSOLE.print("Read PZEM,custom address:");
  //CONSOLE.println(pzem.readAddress(), HEX);

  // Read the data from the sensor
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  freq = pzem.frequency();
  pwfactor = pzem.pf();

  rc=true;
  // Check if the data is valid
  if(isnan(voltage)){
    CONSOLE.println("Error reading voltage");
    voltage=0;
    rc=false;
  } else if (isnan(current)) {
    CONSOLE.println("Error reading current");
    current=0;        
    rc=false;
  } else if (isnan(power)) {
    CONSOLE.println("Error reading power");
    power=0;
    rc=false;
  } else if (isnan(energy)) {
    CONSOLE.println("Error reading energy");
    energy=0;
    rc=false;
  } else if (isnan(freq)) {
    CONSOLE.println("Error reading frequency");
    freq=0;
    rc=false;
  } else if (isnan(pwfactor)) {
    CONSOLE.println("Error reading power factor");
    rc=false;
    pwfactor=0;
  }

  if (! rc){
    u8x8.clearDisplay();
    u8x8.setCursor(5,2);
    u8x8.print("Error!");
    memset(screen_prev,0,sizeof(screen_prev));
    delay(MAIN_DELAY);
    return;
  }

  CONSOLE.print("Voltage: "); CONSOLE.println(voltage); // V
  CONSOLE.print("Current: "); CONSOLE.println(current,3); // A
  CONSOLE.print("Power: "); CONSOLE.println(power); // W
  CONSOLE.print("Energy: "); CONSOLE.println(energy,3); // kWh
  CONSOLE.print("Frequency: "); CONSOLE.println(freq); // Hz
  CONSOLE.print("PowerFactor: "); CONSOLE.println(pwfactor);

  fill_screen();
  draw_screen();
  
#ifdef WIFI_ENABLE
  CONSOLE.print("Counter="); CONSOLE.println(cnt);
  main_buffer[cnt][0]=voltage;
  main_buffer[cnt][1]=current;
  main_buffer[cnt][2]=power;
  main_buffer[cnt][3]=energy;
  main_buffer[cnt][4]=freq;
  main_buffer[cnt][5]=pwfactor;
  cnt++;
  if (cnt == MEASUREMENTS) {
    cnt=0;
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
  }
#endif
  //CONSOLE.println("End of loop, sleeping...");
  delay(MAIN_DELAY);
}

void fill_screen(){
  memset(screen_cur,0,sizeof(screen_cur));

  strncpy(screen_cur[0],"  Measurement  ",LCD_COLS);
  strncat(screen_cur[0],roller+roll_cnt++,1);
  
  if ( roll_cnt >= sizeof(roller) ) roll_cnt=0;

  dtostrf(power,7,2,screen_cur[1]);
  strncat(screen_cur[1],"W (",LCD_COLS);
  dtostrf(pwfactor,4,2,screen_cur[1]+strlen(screen_cur[1]));
  strncat(screen_cur[1],")",LCD_COLS);

  dtostrf(voltage,5,1,screen_cur[2]);
  strncat(screen_cur[2],"V ",LCD_COLS);
  dtostrf(current,6,3,screen_cur[2]+strlen(screen_cur[2]));
  strncat(screen_cur[2],"A",LCD_COLS);

  dtostrf(freq,4,1,screen_cur[3]);
  strncat(screen_cur[3],"Hz ",LCD_COLS);
  dtostrf(energy,6,3,screen_cur[3]+strlen(screen_cur[3]));
}

void draw_screen(){
  if ( screen_prev[0][0] == 0 ) {
    // draw a full screen
    for (uint8_t i=0; i<LCD_ROWS; i++){
       if (strlen(screen_cur[i]) > 0){
            u8x8.drawString(0,i+i,screen_cur[i]);
       } 
    }
    memcpy(screen_prev,screen_cur,sizeof(screen_prev));
    return;
  }
  
  for ( uint8_t i=0; i < LCD_ROWS; i++ ){
    for ( uint8_t j=0; j < LCD_COLS; j++ ){
      if ( screen_cur[i][j] != screen_prev[i][j] ) {
        // update a symbol on the screen
        if ( screen_cur[i][j] != 0 ) {
          screen_prev[i][j] = screen_cur[i][j];
          u8x8.drawGlyph(j,i+i,screen_cur[i][j]);
        } else {
          // clear the rest of the string 
          for ( uint8_t k=j; k < LCD_COLS; k++ ) {
            if ( screen_prev[i][k] == 0 ) {
              break;
            } else {
              if ( screen_prev[i][k] != 32 ) {
                u8x8.drawGlyph(k,i+i,' ');
                screen_prev[i][k]=0;
              }
            }
          }
        }
      }
    }
  } 
}
