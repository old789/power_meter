
void SetSimpleCli(){
  
  cmdSsid = cli.addSingleArgCmd("ssid");
  cmdSsid.setDescription(" Set WiFi SSID");
  
  cmdPassw = cli.addSingleArgCmd("passw");
  cmdPassw.setDescription(" Set WiFi password");
  
  cmdHost = cli.addSingleArgCmd("host");
  cmdHost.setDescription(" Set destination IP address");
  
  cmdPort = cli.addSingleArgCmd("port");
  cmdPort.setDescription(" Set destination port");
  
  cmdPassw = cli.addSingleArgCmd("uri");
  cmdPassw.setDescription(" Set destination URI");
  
  cmdShow = cli.addSingleArgCmd("show");
  cmdShow.setDescription(" Show configuration");
  
  cmdSave = cli.addSingleArgCmd("save");
  cmdSave.setDescription(" Save configuration to EEPROM");
  
  cmdHelp = cli.addSingleArgCmd("help");
  cmdHelp.setDescription(" Get help");
  
}


void  loop_cli_mode(){
  String input;
  Serial.print("> ");
  readStringWEcho(input, MAX_ALLOWED_INPUT);

  if (input.length() > 0) {
    cli.parse(input);
  }

  if (cli.available()) {
    Command c = cli.getCmd();

    uint8_t argNum = c.countArgs();

    if (c == cmdSsid) {
      Serial.println("SSID is \"" + c.getArg(0).getValue() + "\"");
    } else if (c == cmdPassw) {
      Serial.println("Password is \"" + c.getArg(0).getValue() + "\"");
    } else if (c == cmdHost) {
      Serial.println("Host is \"" + c.getArg(0).getValue() + "\"");
    } else if (c == cmdPort) {
      Serial.println("Port is \"" + c.getArg(0).getValue() + "\"");
    } else if (c == cmdUri) {
      Serial.println("URI is \"" + c.getArg(0).getValue() + "\"");
    } else if (c == cmdSave) {
      Serial.println("Configuration saved");
    } else if (c == cmdShow) {
      Serial.println("Show must go on");
    } else if (c == cmdHelp) {
      Serial.println("Help:");
      Serial.println(cli.toString());
    }
    
  }

  if (cli.errored()) {
    CommandError cmdError = cli.getError();

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand()) {
      Serial.print("Did you mean \"");
      Serial.print(cmdError.getCommand().toString());
      Serial.println("\"?");
    }
  }

  // delay(SHORT_DELAY);
}


void readStringWEcho(String& input, size_t char_limit) { // call with char_limit == 0 for no limit
  for(;;) {
    if (Serial.available()) {
      char c = Serial.read();
      if ((uint8_t)c == 8) {
        if ( input.length() ) {
          clearString(input.length());
          input.remove(input.length()-1);
          Serial.print(input);
        }
        continue;
      }
      if ( ((uint8_t)c == 10) || ((uint8_t)c == 13) ){
        Serial.println();
        return;
      }
      if ( ((uint8_t)c < 32) || ((uint8_t)c > 126)) {
        Serial.print((char)7);
        continue;
      }
      input += c;
      Serial.print(c);
      if (char_limit && (input.length() >= char_limit)) {
        return;
      }
    }
  }
}

void clearString( uint16_t len ){
  char stmp[MAX_ALLOWED_INPUT+7];
  memset(stmp+1,' ',len+2);
  stmp[0]='\r';
  stmp[len+3]='\r';
  stmp[len+4]='>';
  stmp[len+5]=' ';
  stmp[len+6]=0;
  Serial.print(stmp);
}
