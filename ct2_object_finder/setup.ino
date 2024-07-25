

// Number of bytes required for the recording buffer
uint32_t record_size = (SAMPLE_RATE * SAMPLE_BITS / 8) * RECORD_TIME;
uint32_t buffer_size = 512;

void init_i2s() {
  uint32_t sample_size = 0;
  esp_err_t err;
  rec_buffer0 = (uint8_t *)ps_malloc(buffer_size);
  rec_buffer1 = (uint8_t *)ps_malloc(buffer_size);
  rec_buffer = (uint8_t *)ps_malloc(record_size);
  audio_buffer = (uint8_t *)ps_malloc(record_size);
  // Write the header to the WAV file
  uint8_t wav_header[WAV_HEADER_SIZE];

  init_record();
  Serial.println("after init record");
  // Write the WAV file header information to the wav_header array
  generate_wav_header(wav_header, record_size, SAMPLE_RATE);
  Serial.println("after wav header");

  if (rec_buffer == NULL) {
    Serial.printf("malloc failed!\n");
  }
  Serial.println("after check buffer");
  Serial.printf("Buffer: %d bytes\n", ESP.getPsramSize() - ESP.getFreePsram());
}

void init_record() {
  esp_err_t err = ESP_OK;
  err = i2s_driver_uninstall(I2S_NUM_0);
  if (err != ESP_OK) {
    Serial.printf("Failed disinstalling driver: %d\n", err);
  }

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    //.channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,     // Also works
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    // .communication_format = I2S_COMM_FORMAT_PCM,            // Also works
    .communication_format = (I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_PIN_NO_CHANGE,
    .ws_io_num = 42,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = 41
  };
  Serial.println("pre install");
  err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed installing INPUT driver: %d\n", err);
    while (true)
      ;
  }
  Serial.println("after install");
  err = i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
  if (err != ESP_OK) {
    Serial.printf("Failed setting up pin INPUT pin driver: %d\n", err);
    while (true)
      ;
  }
  Serial.println("after set pin");
  err = i2s_zero_dma_buffer((i2s_port_t)0);
  Serial.println("after zero buffer");
  if (err != ESP_OK) {
    Serial.printf("Error in initializing dma buffer with 0");
  }
  Serial.println("Pre return");
  return;
}

void init_play() {
  esp_err_t err = ESP_OK;
  i2s_driver_uninstall(I2S_NUM_0);
  i2s_config_t i2s_config_play = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t i2s_play_pins = {
    .bck_io_num = 5,
    .ws_io_num = 4,
    .data_out_num = 6,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  err = i2s_driver_install(I2S_NUM_0, &i2s_config_play, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed installing OUTPUT driver: %d\n", err);
    while (true)
      ;
  }
  err = i2s_set_pin(I2S_NUM_0, &i2s_play_pins);
  if (err != ESP_OK) {
    Serial.printf("Failed setting up pin OUTPUT driver: %d\n", err);
    while (true)
      ;
  }
  return;
}
