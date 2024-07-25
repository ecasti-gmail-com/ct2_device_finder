bool check_cache(void) {
  Serial.println("Checking cache");
  return SD.exists(cachefile.filename);
}


bool read_cache() {
  File myFile;
  myFile = SD.open(cachefile.filename);
  Serial.print("Filename: ");
  Serial.print(cachefile.filename);
  if (myFile) {
    audio_ptr = 0;
    Serial.println("File open: ");
    audio_ptr = (long)myFile.read(audio_buffer, record_size);
    myFile.close();
    Serial.print("File in memory, size: ");
    Serial.println(audio_ptr);


    if (audio_ptr > 0) {
      i2s_write(I2S_NUM_0,
                audio_buffer + 44,
                audio_ptr - 44,  // Number of bytes
                &bytes_read,
                portMAX_DELAY);  // No timeout
                 delay(250);
    } else {
      Serial.println("NO data");
    }
  }
  Serial.println("Play from cache done");
  return true;
}

bool store_cache(void) {
  Serial.printf("Creating Dir: %s\n", cachefile.folder);
  Serial.println("");
  Serial.println(cachefile.folder);
  if (SD.mkdir(cachefile.folder)) {
    Serial.println("Dir created");
  } else {
    Serial.println("Mkdir failed");
  }
  if (SD.mkdir(cachefile.subfolder)) {
    Serial.println("Sub Dir created");
  } else {
    Serial.println("Mkdir sub dir failed");
  }
  Serial.printf("Storing File : %s\n", cachefile.filename);
  File file = SD.open(cachefile.filename, FILE_WRITE);

  if (file.write(audio_buffer, audio_ptr) != audio_ptr) {
    Serial.printf("Write file Failed!\n");
  }

  file.close();
  delay(200);
  return true;
}

void get_md5(const char *payload) {
  byte shaResult[32];
  char str[3];
  char result[64];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  const size_t payloadLength = strlen(payload);

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *)payload, payloadLength);
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);

  Serial.print("Hash: ");

  for (int i = 0; i < sizeof(shaResult); i++) {


    sprintf(str, "%02x", (int)shaResult[i]);
    result[i * 2] = str[0];
    result[(i * 2) + 1] = str[1];
    Serial.print(str);
  }
  Serial.println();
  cachefile.folder[0] = '/';
  cachefile.subfolder[0] = '/';
  cachefile.filename[0] = '/';

  cachefile.folder[1] = result[0];
  cachefile.subfolder[1] = result[0];
  cachefile.filename[1] = result[0];

  cachefile.folder[2] = result[1];
  cachefile.subfolder[2] = result[1];
  cachefile.filename[2] = result[1];
  cachefile.folder[3] = '\0';
  cachefile.subfolder[3] = '/';
  cachefile.filename[3] = '/';
  cachefile.subfolder[4] = result[2];
  cachefile.filename[4] = result[2];
  cachefile.subfolder[5] = result[3];
  cachefile.filename[5] = result[3];
  cachefile.subfolder[6] = '\0';
  cachefile.filename[6] = '/';

  for (int i = 4; i < 16; i++) {
    cachefile.filename[i + 3] = result[i];
  }
  cachefile.filename[19] = '.';
  cachefile.filename[20] = 'w';
  cachefile.filename[21] = 'a';
  cachefile.filename[22] = 'v';
  cachefile.filename[23] = '\0';
  return;  //result;
}