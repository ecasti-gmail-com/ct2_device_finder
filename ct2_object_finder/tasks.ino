void wifiConnect(void *pvParameters) {
  isWIFIConnected = false;
  Serial.print("Try to connect to ");
  Serial.println(ssidName);
  WiFi.begin(ssidName, ssidPswd);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500);
    Serial.print(".");
  }
  Serial.println("Wi-Fi Connected!");
  isWIFIConnected = true;
    Serial.println ( "" );

  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );
  // Ignore SSL certificate validation
  init_webserver();
  client.setInsecure();
  while (true) {
    vTaskDelay(1000);
  }
}
/*
Continuosly record the audio to the buffer, if the BUTTON1 is pressed,
until the max of 10s
The buffer is fixed and recycled, so we don't have to generate the header every time, 
And we don't have to allocate again the memory
*/
void i2s_adc(void *arg) {

  size_t sample_size = 0;
  Serial.println("Init Task");
  for (;;) {
    if ((digitalRead(BUTTON_VOICE) == 0) && (function > 0)&& (function < 4) ) {

      // check if was not recording already
      if (ONREC == 0) {
        // Start recording
        init_record();
        Serial.println("After Init_record");
        ONREC = 1;
        data_ptr = WAV_HEADER_SIZE;
        DATA_READY = 0;
        buffer0_ready = 0;
        buffer1_ready = 0;
        buffn = 0;
      }
      // continue recording
      else {
        if (buffn == 0) {
          buffn = 1;
          // esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, rec_buffer1, buffer_size, &sample_size, portMAX_DELAY);
          i2s_read(I2S_NUM_0, rec_buffer1, buffer_size, &sample_size, portMAX_DELAY);
          if (debug) {
            Serial.print("Buffer1: ");
            Serial.println(sample_size);
          }
          buffer1_ready = 1;

        } else {
          buffn = 0;
          i2s_read(I2S_NUM_0, rec_buffer0, buffer_size, &sample_size, portMAX_DELAY);
          if (debug) {
            Serial.print("Buffer0: ");
            Serial.println(sample_size);
          }
          buffer0_ready = 1;
        }
      }
    } else
    // not recording
    {
      // Check if we need to set the flag
      if (ONREC == 1) {
        // reset the flag
        ONREC = 0;
        DATA_READY = 1;
      }
      vTaskDelay(10);
    }
  }
  //  vTaskDelete(NULL);
}