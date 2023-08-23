
#define PZEM004_NO_SWSERIAL
#include <PZEM004Tv30.h>  // https://github.com/mandulaj/PZEM-004T-v30

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#ifndef PZEM004_NO_SWSERIAL
#include <SoftwareSerial.h>
#endif

#include <U8x8lib.h>    // https://github.com/olikraus/u8g2

#define SCL_PIN SCL  // SCL pin of OLED. Default: D1 (ESP8266) or D22 (ESP32)
#define SDA_PIN SDA  // SDA pin of OLED. Default: D2 (ESP8266) or D21 (ESP32)
#define LCD_COLS 16
#define LCD_ROWS 4
#define MEASUREMENTS 30
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

//WiFi data
char ssid[] = "SSID"; //WiFi Credential
char pass[] = "PASSW"; //WiFi Password

//Domain name with URL
const char* serverName = "http://10.10.10.10/pwr/pwrm.php";

WiFiClient client;

double voltage, current, power, energy, freq, pwfactor;
unsigned long upcounter=0;
bool rc=false;
uint8_t roll_cnt=0;
char roller[] = { '-', '/', '|', '\\' };
bool enable_collect_data=false;

char str_voltage[8];
char str_current[8];
char str_power[16];
char str_energy[16];
char str_freq[8];
char str_pfactor[8];
char str_tmp[64];

char screen_cur[LCD_ROWS][LCD_COLS+1];
char screen_prev[LCD_ROWS][LCD_COLS+1];

int cnt=0;
char str_post[2048];

void setup(){
 pinMode(D5, INPUT_PULLUP); 
 pinMode(D6, INPUT_PULLUP); 
#ifdef PZEM004_NO_SWSERIAL
  Serial.swap();
#endif
  CONSOLE.begin(115200);
  delay(50);
  CONSOLE.println(".\nStart serial");

  // if ( !digitalRead(D6) ){
    enable_collect_data=true;
  // }
  
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
 
  if (enable_collect_data) {
    wifi_init();
    memset(str_post,0,sizeof(str_post));
  }
}

void loop(){

  CONSOLE.println("\n");  
  CONSOLE.print("Round "); CONSOLE.println(upcounter++);
  
  rc = read_pzem();
  if (rc){
    fill_screen();
    draw_screen();
    if ( enable_collect_data ) {
      collect_data();
    }
  }else{
    u8x8.clearDisplay();
    u8x8.drawString(1,2,"!Sensor Error!");
    memset(screen_prev,0,sizeof(screen_prev));
  }

  //CONSOLE.println("End of loop, sleeping...");
  delay(MAIN_DELAY);
}

void collect_data(){
  CONSOLE.print("Counter="); CONSOLE.println(cnt);
  
  dtostrf(voltage,1,1,str_voltage);
  dtostrf(current,1,3,str_current);
  dtostrf(power,1,1,str_power);
  dtostrf(energy,1,3,str_energy);
  dtostrf(freq,1,1,str_freq);
  dtostrf(pwfactor,1,2,str_pfactor);
  sprintf(str_tmp,"m%u=%s,%s,%s,%s,%s,%s,%u&",cnt,str_voltage,str_current,str_power,str_energy,str_freq,str_pfactor,millis());
  if ( strlen(str_post) + strlen(str_tmp) >= sizeof(str_post)-1 ) {
     CONSOLE.println("str_post is too short");
     cnt = MEASUREMENTS;
  }else{
    if (cnt > 0){
      strncat(str_post,str_tmp,sizeof(str_post)-1);
    }else{
      strncpy(str_post,str_tmp,sizeof(str_post)-1);
    }
  }

  CONSOLE.print("Length of buffer="); CONSOLE.println(strlen(str_post));
  
  if (++cnt >= MEASUREMENTS) {
    send_data();
    cnt=0;
    memset(str_post,0,sizeof(str_post));
  }
}

void send_data(){
  CONSOLE.println("Send data");
  strncpy(screen_cur[0],"Sending data...",LCD_COLS);
  draw_screen();
  //Check WiFi connection status
  if(WiFi.status() != WL_CONNECTED){
    CONSOLE.println("WiFi Disconnected");
    u8x8.clearDisplay();
    memset(screen_prev,0,sizeof(screen_prev));
    wifi_init();
  }
  //CONSOLE.println("HTTP client");
  HTTPClient http;

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
  sprintf(screen_cur[0],"Data sent: %d",httpResponseCode);
  draw_screen();
  
  CONSOLE.print("HTTP Response code: "); CONSOLE.println(httpResponseCode);
  CONSOLE.println("Free resources");
  
  // Free resources
  http.end();
}

void wifi_init(){
  CONSOLE.print("Connecting to ");
  CONSOLE.print(ssid);
  CONSOLE.println(" ...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);             // Connect to the network 

  u8x8.drawString(0,2,"WiFi connecting");

  uint16_t i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    u8x8.drawGlyph(7,4,uint8_t(roller[roll_cnt++]));
    if ( roll_cnt >= sizeof(roller) ) roll_cnt=0;
    delay(1000);
    i++;
    CONSOLE.print(i); CONSOLE.print(' ');
    if ( i > 300 ) {  // if don't connect then restart
      u8x8.clearDisplay();
      u8x8.drawString(2,2,"WiFi timeout");
      u8x8.drawString(3,4,"Restarting");
      delay(3000);
      ESP.restart();
    }
  }
  
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  CONSOLE.println('\n');
  CONSOLE.println("Connection established!");  
  CONSOLE.print("IP address: ");CONSOLE.println(WiFi.localIP());
  CONSOLE.print("RSSI: ");CONSOLE.println(WiFi.RSSI());
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

bool read_pzem(){
  CONSOLE.println("Read PZEM");  

  //CONSOLE.print("Read PZEM,custom address:");
  //CONSOLE.println(pzem.readAddress(), HEX);

  // Read the data from the sensor

  voltage = pzem.voltage();
  if(isnan(voltage)){
    CONSOLE.println("Error reading voltage");
    return(false);
  }
  
  current = pzem.current();
  if (isnan(current)) {
    CONSOLE.println("Error reading current");  
    return(false);
  } 
  
  power = pzem.power();
  if (isnan(power)) {
    CONSOLE.println("Error reading power");
    return(false);
  }

  energy = pzem.energy();
  if (isnan(energy)) {
    CONSOLE.println("Error reading energy");
    return(false);
  } 

  freq = pzem.frequency();
  if (isnan(freq)) {
    CONSOLE.println("Error reading frequency");
    return(false);
  } 

  pwfactor = pzem.pf();
  if (isnan(pwfactor)) {
    CONSOLE.println("Error reading power factor");
    return(false);
  }

  CONSOLE.print("Voltage: "); CONSOLE.println(voltage); // V
  CONSOLE.print("Current: "); CONSOLE.println(current,3); // A
  CONSOLE.print("Power: "); CONSOLE.println(power); // W
  CONSOLE.print("Energy: "); CONSOLE.println(energy,3); // kWh
  CONSOLE.print("Frequency: "); CONSOLE.println(freq); // Hz
  CONSOLE.print("PowerFactor: "); CONSOLE.println(pwfactor);

  return(true);
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
