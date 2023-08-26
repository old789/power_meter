#define DEBUG_SERIAL  // because just "DEBUG" defined in PZEM004Tv30.h
//#define DEBUG_SENSOR
#define DBG_WIFI    // because "DEBUG_WIFI" deifined in a WiFiClient library 

#if defined ( DEBUG_SENSOR ) && not defined ( DEBUG_SERIAL )
#define DEBUG_SERIAL
#endif

#if defined ( DBG_WIFI ) && not defined ( DEBUG_SERIAL )
#define DEBUG_SERIAL
#endif

#define PZEM004_NO_SWSERIAL
#include <PZEM004Tv30.h>  // https://github.com/mandulaj/PZEM-004T-v30

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#ifndef PZEM004_NO_SWSERIAL
#include <SoftwareSerial.h>
#endif

#include <U8x8lib.h>    // https://github.com/olikraus/u8g2

#include <SimpleCLI.h>  // https://github.com/SpacehuhnTech/SimpleCLI

#define SCL_PIN SCL  // SCL pin of OLED. Default: D1 (ESP8266) or D22 (ESP32)
#define SDA_PIN SDA  // SDA pin of OLED. Default: D2 (ESP8266) or D21 (ESP32)
#define LCD_COLS 16
#define LCD_ROWS 4
#define MEASUREMENTS 30
#define MAIN_DELAY 1000
#define SHORT_DELAY MAIN_DELAY/10
#define MAX_ALLOWED_INPUT 127

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

// Create CLI Object
SimpleCLI cli;

//WiFi data
char ssid[] = "SSID"; //WiFi Credential
char pass[] = "PASSW"; //WiFi Password

//Domain name with URL
const char* serverName = "http://10.10.10.10/pwr/pwrm.php";

WiFiClient client;

double voltage, current, power, energy, freq, pwfactor;
unsigned long upcounter=0;
unsigned long ticks_sleep=0;
unsigned long ticks_start=0;
unsigned long ticks_last=0;
bool rc=false;
uint8_t roll_cnt=0;
char roller[] = { '-', '/', '|', '\\' };
bool enable_collect_data=false;
bool enable_cli=false;

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

// Commands
Command cmdSsid;
Command cmdPassw;
Command cmdShow;
Command cmdHost;
Command cmdPort;
Command cmdUri;
Command cmdSave;
Command cmdHelp;

void setup(){
  pinMode(D5, INPUT_PULLUP); 
  pinMode(D6, INPUT_PULLUP); 

  // initialize OLED
  u8x8.begin();
  // u8x8.setBusClock(400000);
  u8x8.setFont(u8x8_font_8x13_1x2_f);

  if ( digitalRead(D5) ){
    // Command line mode
    enable_cli=true;
    u8x8.drawString(0, 2, "CommandLine Mode");
    Serial.begin(115200);
    delay(50);
    Serial.println("CommandLine Mode");
    SetSimpleCli();
  }else{
    // usual mode
    if ( !digitalRead(D6) ){
      enable_collect_data=true;
    }
  
#ifdef PZEM004_NO_SWSERIAL
    Serial.swap();
#endif

#ifdef DEBUG_SERIAL
    CONSOLE.begin(115200);
    delay(50);
    CONSOLE.println(".\nStart serial");
#endif

    u8x8.drawString(1, 0, "Booting...");
    memset(screen_prev,0,sizeof(screen_prev));
  
    // clear PZEM energy counter
    energy = pzem.energy();
    if ( ( !isnan(energy) ) && ( energy > 0 ) ) {
#ifndef DEBUG_SENSOR
      pzem.resetEnergy();
#else
      if ( pzem.resetEnergy()) {
        CONSOLE.println("Reset energy counter");
      }else{
        CONSOLE.println("Can't reset energy counter");
      }
#endif
    }
  }
 
  if (enable_collect_data) {
    wifi_init();
    memset(str_post,0,sizeof(str_post));
  }
}

void loop(){
  if (enable_cli) {
    loop_cli_mode();
  }else{
    loop_usual_mode();
  }
}

void loop_usual_mode(){
  ticks_start=millis();
#if defined ( DEBUG_SENSOR ) || defined ( DBG_WIFI )
  CONSOLE.println();
#endif  
#ifdef DEBUG_SERIAL
  CONSOLE.print("Round "); CONSOLE.println(upcounter++);
#endif
  
  rc = read_pzem();
  if (rc){
    fill_screen();
    draw_screen();
    if ( enable_collect_data ) {
      collect_data();
    }
  }else{
    memset(screen_cur,0,sizeof(screen_cur));
    strncpy(screen_cur[1]," !Sensor Error!",LCD_COLS);
    if ( enable_collect_data && ( uint8_t(str_post[0]) != 0 ) ) { // data send emergency
      send_data();
    }else{
      draw_screen();
    }
  }
  ticks_sleep=neat_interval(ticks_start);
#ifdef DEBUG_SERIAL
  CONSOLE.print("End of loop, sleeping for ");CONSOLE.print(ticks_sleep);CONSOLE.println("ms");
#endif
  delay(ticks_sleep);
}

unsigned long neat_interval( unsigned long ticks_start ){
unsigned long ticks_now=millis();
unsigned long res = 0;

  if ( ticks_now < ticks_start ) {
    res = 0xffffffffffffffff - ticks_start + ticks_now;
  }else{
    res = ticks_now - ticks_start;
  }
  if ( res >= MAIN_DELAY ) {  // it is possible in case with net timeout
    return(SHORT_DELAY);
  }
  return( MAIN_DELAY - res );
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
#ifdef DEBUG_SENSOR
  CONSOLE.println("Read PZEM");  
  //CONSOLE.print("Read PZEM,custom address:"); CONSOLE.println(pzem.readAddress(), HEX);
#endif

  // Read the data from the sensor

  voltage = pzem.voltage();
  if(isnan(voltage)){
#ifdef DEBUG_SENSOR
    CONSOLE.println("Error reading voltage");
#endif
    return(false);
  }
  
  current = pzem.current();
  if (isnan(current)) {
#ifdef DEBUG_SENSOR
    CONSOLE.println("Error reading current");  
#endif
    return(false);
  } 
  
  power = pzem.power();
  if (isnan(power)) {
#ifdef DEBUG_SENSOR
    CONSOLE.println("Error reading power");
#endif
    return(false);
  }

  energy = pzem.energy();
  if (isnan(energy)) {
#ifdef DEBUG_SENSOR
    CONSOLE.println("Error reading energy");
#endif
    return(false);
  } 

  freq = pzem.frequency();
  if (isnan(freq)) {
#ifdef DEBUG_SENSOR
    CONSOLE.println("Error reading frequency");
#endif
    return(false);
  } 

  pwfactor = pzem.pf();
  if (isnan(pwfactor)) {
#ifdef DEBUG_SENSOR
    CONSOLE.println("Error reading power factor");
#endif
    return(false);
  }

#ifdef DEBUG_SENSOR
  CONSOLE.print("Voltage: "); CONSOLE.println(voltage); // V
  CONSOLE.print("Current: "); CONSOLE.println(current,3); // A
  CONSOLE.print("Power: "); CONSOLE.println(power); // W
  CONSOLE.print("Energy: "); CONSOLE.println(energy,3); // kWh
  CONSOLE.print("Frequency: "); CONSOLE.println(freq); // Hz
  CONSOLE.print("PowerFactor: "); CONSOLE.println(pwfactor);
#endif

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
