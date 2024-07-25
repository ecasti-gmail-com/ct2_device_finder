void sendrequest() {
  getAll = "";
  getBody = "";
  String head = "--Build2Gether\r\nContent-Disposition: form-data; name=\"audio_file\"; filename=\"aaa.mp3\"\r\nContent-Type:  multipart/form-data\r\n\r\n";
  String tail = "\r\n--Build2Gether--\r\n";
  String serverName = URL_SERVER;  //REPLACE WITH YOUR DOMAIN NAME
  String serverPath = URL_PATH;    // The default serverPath should be upload.php

  const int serverPort = 443;  //server port for HTTPS
  uint32_t imageLen = data_ptr;
  uint32_t extraLen = head.length() + tail.length();
  uint32_t totalLen = imageLen + extraLen;
  Serial.println("Sending http request");



  client.setInsecure();  //skip certificate validation
  if (client.connect(serverName.c_str(), serverPort)) {
    client.println("POST " + serverPath + " HTTP/1.0");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=Build2Gether");
    client.println();
    client.print(head);

    for (size_t n = 0; n < data_ptr; n = n + 1024) {
      if (n + 1024 < data_ptr) {
        client.write(rec_buffer, 1024);
        rec_buffer += 1024;
      } else if (data_ptr % 1024 > 0) {
        size_t remainder = data_ptr % 1024;
        client.write(rec_buffer, remainder);
      }
    }
    client.print(tail);
    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length() == 0) { state = true; }
          getAll = "";
        } else if (c != '\r') {
          getAll += String(c);
        }
        if (state == true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length() > 0) { break; }
    }
    Serial.println();
    client.stop();

    //Serial.println(getAll);
    //Serial.println(getBody);
  } else {
    getBody = "Connection to " + serverName + " failed.";
    Serial.println(getBody);
  }
}
/*
Play the expected audio from cache, if available, 
otherwise will download from Marytts and play from memory. 
*/
void play_audio(String str_in) {
  get_md5(str_in.c_str());
  init_play();
  delay(200);
  /*



  */
  if (check_cache()) {
    Serial.println("Data in cache");
    read_cache();

  } else {
    /*
      Download from http and store
    */
    Serial.println("Data not in cache");
    audio_ptr = 0;
    String serverName = URL_SERVER;  //
    Serial.println("-----------");

    String encoded_in = urlEncode(str_in);
    // String serverPath = TTS_PATH;    //
    String serverPath = TTS_PATH1 + encoded_in + TTS_PATH2;
    String url = "https://" + serverName + serverPath;
    Serial.print("URL: ");
    Serial.println(url);
    const int serverPort = 443;  //server port for HTTPS
    client.setInsecure();        //skip certificate validation
                                 // new
    HTTPClient https;
    if (https.begin(client, url)) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          Serial.println("Downloaded, ready to copy");
          audio_ptr = https.getSize();
          Serial.printf("File Size: %d\n", audio_ptr);
          audio_buffer = (uint8_t*)https.getString().c_str();
          Serial.println("Copied");
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
    Serial.println();
    client.stop();
    Serial.print("Size: ");
    Serial.println(audio_ptr);
    adj_volume();
    if (audio_ptr > 0) {
      i2s_write(I2S_NUM_0,
                audio_buffer + 44,
                audio_ptr - 44,  // Number of bytes
                &bytes_read,
                portMAX_DELAY);  // No timeout
      //store_cache();
      delay(250);
    }
  }
}