  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

bool eeprom_save(void){
  byte *buf = (byte*)malloc( SIZE_EEPROM + 8 );

  if ( ! buf ){
    Serial.println("Can't allocate memory, condiguration didn't saved");
    return(false);
  }
  
  memset(buf, 0, SIZE_EEPROM + 8);
  memcpy(buf, &mark, sizeof(mark));
  memcpy(buf+PT_MEASUREMENTS, &measurements, sizeof(measurements));
  memcpy(buf+PT_SSID, &ssid, sizeof(ssid));
  memcpy(buf+PT_PASSW, &passw, sizeof(passw));
  memcpy(buf+PT_HOST, &host, sizeof(host));
  memcpy(buf+PT_PORT, &port, sizeof(port));
  memcpy(buf+PT_URI, &uri, sizeof(uri));

  
  EEPROM.put(0, mark);
  EEPROM.put(PT_MEASUREMENTS, measurements);
  EEPROM.put(PT_SSID, ssid);
  EEPROM.put(PT_PASSW, passw);
  EEPROM.put(PT_HOST, host);
  EEPROM.put(PT_PORT, port);
  EEPROM.put(PT_URI, uri);
  // EEPROM.put(PT_CRC, crc);
  EEPROM.commit();

  Serial.print("EEPROM CRC = "); Serial.println(eeprom_crc(SIZE_EEPROM),HEX);
  Serial.print("RAM CRC = "); Serial.println(ram_crc(buf,SIZE_EEPROM),HEX);
  free(buf);
  return(true);
}

unsigned long eeprom_crc(uint16_t eeprom_length) {
  unsigned long crc = ~0L;

  for (uint16_t index = 0 ; index < eeprom_length  ; ++index) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }

  return crc;
}

unsigned long ram_crc(byte *buf, uint16_t buf_size) {
  unsigned long crc = ~0L;

  for (uint16_t index = 0 ; index < buf_size  ; ++index) {
    crc = crc_table[(crc ^ buf[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (buf[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }

  return crc;
}

void eeprom_read(){
uint16_t m = 0;

  EEPROM.get(0, m);
  EEPROM.get(PT_MEASUREMENTS, measurements);
  EEPROM.get(PT_SSID, ssid);
  EEPROM.get(PT_PASSW, passw);
  EEPROM.get(PT_HOST, host);
  EEPROM.get(PT_PORT, port);
  EEPROM.get(PT_URI, uri);
  // EEPROM.get(PT_CRC, crc);

  if ( m == mark ) {
    eeprom_valid=true;
    if ( enable_cli ) {
      Serial.println("EEPROM read successfully");
    }
  }else if ( enable_cli ) {
    Serial.print("EEPROM read ERROR: need 0x");Serial.print(mark,HEX);Serial.print(" read 0x");Serial.print(m,HEX);
  }

}
