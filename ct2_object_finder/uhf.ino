/*
 Scan to find some object from the array codes

*/
void antenna_find() {
  uint8_t result = 0;
  int scan_timeout = 300 * 1000;
  int tf = millis() + scan_timeout;
  int a = 0;
  int b = 0;
  while ((millis() < tf) && (digitalRead(BUTTON_FIND) != 0))  {
    result = uhf.pollingOnce();
    if (result > 0) {
      for (a = 0; a < result; a++) {
        for (b = 0; b < codes_p; b++) {
          if (uhf.cards[a].epc_str == codes[b]) {
            Serial.println(uhf.cards[a].epc_str);
            sound_beep();
          }
        }
      }
    }
    delay(25);
  }
}

/*
 Identify the objects on the range
*/
void antenna_scan() {
  uint8_t result = 0;
  codes_p = 0;
  int scan_timeout = 20 * 1000;
  int tf = millis() + scan_timeout;
  int a = 0;
  while (millis() < tf) {
    result = uhf.pollingOnce();
    if (result > 0) {
      for (a = 0; a < result; a++) {
        Serial.print("Code to find: ");
        Serial.print(uhf.cards[a].epc_str);
        if (db_get_object_name(uhf.cards[a].epc_str, true)) {
          Serial.println(": Found ");
        } else {
          Serial.println(": Not Found ");
        }
      }
    }
    delay(50);
  }
}

/*
 Get the first entry and exit
*/
void antenna_scan_once() {
  uint8_t result = 0;
  codes_p = 0;
  char tmpstr[64];
  while (result == 0) {
    result = uhf.pollingOnce();
    if (result > 0) {
      for (uint8_t i = 0; i < result; i++) {
        Serial.println(uhf.cards[i].epc_str);
        // sprintf(tmpstr, "%s", uhf.cards[i].epc_str +  '\0');
        codes[codes_p++] = uhf.cards[i].epc_str + '\0';
      }
    }
    delay(10);
  }
  sound_beep();
}