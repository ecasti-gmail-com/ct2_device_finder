//*****************************************Audio Process******************************************//

struct soundhdr {
  char riff[4];         /* "RIFF"                                  */
  long flength;         /* file length in bytes                    */
  char wave[4];         /* "WAVE"                                  */
  char fmt[4];          /* "fmt "                                  */
  long chunk_size;      /* size of FMT chunk in bytes (usually 16) */
  short format_tag;     /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM */
  short num_chans;      /* 1=mono, 2=stereo                        */
  long srate;           /* Sampling rate in samples per second     */
  long bytes_per_sec;   /* bytes per second = srate*bytes_per_samp */
  short bytes_per_samp; /* 2=16-bit mono, 4=16-bit stereo          */
  short bits_per_samp;  /* Number of bits per sample               */
  char data[4];         /* "data"                                  */
  long dlength;         /* data length in bytes (filelength - 44)  */
} wavh;

void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate) {
  // See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
  uint32_t file_size = wav_size + WAV_HEADER_SIZE - 8;
  uint32_t byte_rate = SAMPLE_RATE * SAMPLE_BITS / 8;
  Serial.println("Before Assign");
  // New format
  strncpy(wavh.riff, "RIFF", 4);
  strncpy(wavh.wave, "WAVE", 4);
  strncpy(wavh.fmt, "fmt ", 4);
  strncpy(wavh.data, "data", 4);

  // size of FMT chunk in bytes
  wavh.chunk_size = 16;
  wavh.format_tag = 1;  // PCM
  wavh.num_chans = 1;   // mono
  // This is easier than converting to hex and then to bytes :)
  wavh.srate = sample_rate;
  wavh.bits_per_samp = 16;
  wavh.flength = file_size;
  wavh.dlength = wav_size;
  memorycopy((byte *)&wavh, rec_buffer, 44, 0);
}

void get_audio(String *str_text) {
  // String str_text;
  String hash_s;
  // Await for data ready
  while (DATA_READY == 0) {
    if (buffer1_ready == 1) {
      if (debug) {
        Serial.println("Buffer 1");
      }
      memorycopy(rec_buffer1, rec_buffer, 512, data_ptr);
      data_ptr += 512;
      buffer1_ready = 0;
    }
    if (buffer0_ready == 1) {
      if (debug) {
        Serial.println("Buffer 0");
      }
      memorycopy(rec_buffer0, rec_buffer, 512, data_ptr);
      data_ptr += 512;
      buffer0_ready = 0;
    }
    delay(5);
  }
  Serial.println("Exit from Loop");

  // increase volume
  for (uint32_t i = 0; i < data_ptr; i += SAMPLE_BITS / 8) {
    (*(uint16_t *)(rec_buffer + i)) <<= VOLUME_GAIN;
  }
  Serial.println("received data");
  generate_wav_header(wav_header, data_ptr, SAMPLE_RATE);

  init_play();
  i2s_write(I2S_NUM_0,
            rec_buffer + 44,
            data_ptr - 44,  // Number of bytes
            &bytes_read,
            portMAX_DELAY);  // No timeout
  sendrequest();
  Serial.println(getBody);
  DeserializationError error = deserializeJson(doc, getBody);
  Serial.println(error.c_str());
  Serial.println("text");

  *str_text = doc["text"].as<String>();
  Serial.print("str_text: ");
  Serial.println(*str_text);
  int ll = str_text->length();
  Serial.print("String length: ");
  Serial.println(ll);
  str_text->toCharArray(tmp2, ll + 1);
  Serial.print("tmp2: ");
  Serial.println(tmp2);
  // Clean the string
  str_text->trim();
  str_text->toLowerCase();
  for (int st = 0; st < ll; st++) {
    if (!((str_text->charAt(st) >= 'a') && (str_text->charAt(st) <= 'z'))) {
      str_text->setCharAt(st, '_');
    }
  }
  /*str_text->replace(" ", "_");
  str_text.replace("!", "");
  str_text.replace("?", "");
  str_text.replace(",", "");
  str_text.replace(".", "");
  str_text.replace("'", "");
  str_text.replace()

  */
  str_text->replace("_", "");
  str_text->trim();
  str_text->concat('\0');
  Serial.println();
  Serial.print("Output string>>");
  Serial.print(*str_text);
  Serial.println("<<");
  if (*str_text != "") {
    play_audio(*str_text);
  } else {
    play_audio("No command received");
  }
  DATA_READY = 0;
  return;
}

void adj_volume() {
  int16_t SignedSample;
  Serial.println("Apply volume change");
  for (int i = 44; i < audio_ptr; i += 2) {
    SignedSample = *((int16_t *)(audio_buffer + i));  // Get the Byte address, convert to an int pointer, then get contents of that address as an int
    SignedSample = SignedSample * volume_lower;       // multiply by the volume - a value that will be between 0 and 1, 1 would be full vol
    *((int16_t *)(audio_buffer + i)) = SignedSample;  // Store back in the memory location we got the sample from
  }
}

void sound_beep() {
  init_play();
  i2s_write(I2S_NUM_0,
            sound_beep_data + 44,
            9658,  // Number of bytes
            &bytes_read,
            portMAX_DELAY);  // No timeout
  delay(100);
  i2s_driver_uninstall(I2S_NUM_0);
}

void sound_error() {
  init_play();
  i2s_write(I2S_NUM_0,
            sound_error_data + 44,
            63084,  // Number of bytes
            &bytes_read,
            portMAX_DELAY);  // No timeout
  delay(100);
  i2s_driver_uninstall(I2S_NUM_0);
}