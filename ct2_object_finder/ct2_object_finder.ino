#include "config.h"
#include <driver/i2s.h>
#include <soc/i2s_reg.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UrlEncode.h>
#include <ArduinoJson.h>
#include "UNIT_UHF_RFID.h"
#include "mbedtls/md.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <sqlite3.h>
#include "audio_sample.h"
#include <WebServer.h>

#define SAMPLE_RATE 16000U
#define SAMPLE_BITS 16
#define WAV_HEADER_SIZE 44
#define VOLUME_GAIN 2
#define RECORD_TIME 40  // seconds, The maximum value is 240
#define debug 0

JsonDocument doc;
WiFiClientSecure client;
Unit_UHF_RFID uhf;
sqlite3 *db1;
sqlite3_stmt *res;
TaskHandle_t Task1;
TaskHandle_t Task2;
WebServer server(80);


struct WavHeader_Struct {
  //   RIFF Section
  char RIFFSectionID[4];  // Letters "RIFF"
  uint32_t Size;          // Size of entire file less 8
  char RiffFormat[4];     // Letters "WAVE"

  //   Format Section
  char FormatSectionID[4];  // letters "fmt"
  uint32_t FormatSize;      // Size of format section less 8
  uint16_t FormatID;        // 1=uncompressed PCM
  uint16_t NumChannels;     // 1=mono,2=stereo
  uint32_t SampleRate;      // 44100, 16000, 8000 etc.
  uint32_t ByteRate;        // =SampleRate * Channels * (BitsPerSample/8)
  uint16_t BlockAlign;      // =Channels * (BitsPerSample/8), effectivly the size of a single sample for all chans.
  uint16_t BitsPerSample;   // 8,16,24 or 32

  // Data Section
  char DataSectionID[4];  // The letters "data"
  uint32_t DataSize;      // Size of the data that follows
} WavHeader;

struct cache {
  char folder[4];
  char subfolder[7];
  char filename[24];
} cachefile;
//MD5  hashMD5;
// Variables to be used in the recording program, do not change for best
String objname;
String getAll;
String getBody;
String getBody2;
//String tmp;
char tmp2[200];
float volume_lower = 1.0;
uint8_t function = 0;
/* High when recording */
bool ONREC = 0;
/* High when ready to send */
bool DATA_READY = 0;
bool buffer0_ready = 0;
bool buffer1_ready = 0;
/* Pointer for end of record */
long data_ptr;
long audio_ptr = 0;
short buffn = 0;
int obid = 0;
bool isWIFIConnected;
uint8_t *rec_buffer0 = NULL;
uint8_t *rec_buffer1 = NULL;
uint8_t *rec_buffer = NULL;
uint8_t *audio_buffer = NULL;

void i2s_adc(void *arg);

uint8_t wav_header[WAV_HEADER_SIZE];

unsigned int bytes_read = 0;
String codes[20];
int codes_p = 0;




void memorycopy(uint8_t *src, uint8_t *dst, long size, long start) {

  for (long i = 0; i < size; i++) {
    *(dst + start + i) = *(src + i);
  }
}


void setup() {
  delay(1000);
  data_ptr = 44;
  pinMode(BUTTON_VOICE, INPUT_PULLUP);
  pinMode(BUTTON_IDENTIFY, INPUT_PULLUP);
  pinMode(BUTTON_FIND, INPUT_PULLUP);
  /*
  pinMode(BUTTONVP, INPUT_PULLUP);
  pinMode(BUTTONVM, INPUT_PULLUP);
  pinMode(BUTTON_ADD, INPUT_PULLUP);
*/
  Serial.begin(115200);
  uhf.begin(&Serial2, 115200, 44, 43, false);
  uhf.setTxPower(2600);
  delay(1000);
  Serial.println("Start Setup");
  init_i2s();
  Serial.println("after init i2s");
  xTaskCreatePinnedToCore(
    i2s_adc,   /* Function to implement the task */
    "i2s_adc", /* Name of the task */
    50000,     /* Stack size in words */
    NULL,      /* Task input parameter */
    0,         /* Priority of the task */
    &Task1,    /* Task handle. */
    0);        /* Core where the task should run */
  Serial.println("after task 1");
  xTaskCreatePinnedToCore(
    wifiConnect,   /* Function to implement the task */
    "wifiConnect", /* Name of the task */
    20000,         /* Stack size in words */
    NULL,          /* Task input parameter */
    0,             /* Priority of the task */
    &Task2,        /* Task handle. */
    0);            /* Core where the task should run */
  Serial.println("after task 2");
  if (!SD.begin(21, SPI, 4000000U, "/sd", 20U, false)) {
    Serial.println("Failed to mount SD Card!");
    while (1)
      ;
  }
  Serial.println("after SD Init");
  // SQL
  sqlite3_initialize();
  Serial.println("after sqlite3_initialize");

  if (openDb("/sd/ctrfid.db", &db1)) {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("setupSDCard PASS . SIZE = ");
    Serial.print(cardSize);
    Serial.println(" MB");
  } else {
    Serial.println("No valid SD Card dedected");
  }

  Serial.println("End Setup");
  delay(3000);
  if (isWIFIConnected) {
    sound_beep();
  }
}

void printwav() {
  long x;
  for (x = 0; x < data_ptr; x += 2) {
    Serial.println(int(*(uint16_t *)(rec_buffer + x)));
  }
}

void printheader() {
  uint8_t x;
  for (x = 0; x < 45; x++) {
    Serial.println(*(uint8_t *)(rec_buffer + x), HEX);
  }
}


void loop() {
  String str_text;
  String hash_s;
  // put your main code here, to run repeatedly:

  /// main workflow
  /*
    Listen for commands
    accept: 
    - (1) find 
    - (2) identify
    - (3) insert 
    - (4) remove
    - (5) battery status
    - (6) assign (tag)
    - (7) alias (add alias for an object)
    - (8) add note ?
    - (9) remove note ?
    - (99) Commands

    */



  while (true) {
    server.handleClient();
    if (digitalRead(BUTTON_VOICE) == 0) {
      function = 4;
    }

    if (digitalRead(BUTTON_IDENTIFY) == 0) {
      delay(250);
      if (digitalRead(BUTTON_FIND) == 0) {
        function = 3;
      } else {
        function = 2;
      }
    }


    if (digitalRead(BUTTON_FIND) == 0) {
      delay(250);
      if (digitalRead(BUTTON_IDENTIFY) == 0) {
        function = 3;
      } else {
        function = 1;
      }
    }
    /*
        if (digitalRead(BUTTON_ADD) == 0) {
      function = 3;
    }
    if (digitalRead(BUTTONVP) == 0) {
      volume_lower++;
      sound_beep;
      if (volume_lower > 8) {
        volume_lower = 8;
      }
    }
    if (digitalRead(BUTTONVM) == 0) {
      volume_lower++;
      sound_beep;
      if (volume_lower < 0) {
        volume_lower = 0;
      }
    }
*/


    switch (function) {
      case 1:
        //set_timer(600);
        // say object name
        play_audio("Which object you want to find?");
        // get object name
        get_audio(&objname);
        obid = db_get_object_id(&objname);
        if (obid > 0) {
          db_search_codes(&objname);
          antenna_find();
          function = 0;
          play_audio("Search completed");
        } else {
          play_audio("Object not found in the database");
        }
        function = 0;
        play_audio("Stop");
        break;
      case 2:
        // set_timer(600);
        play_audio("Identifying the objects in the range");

        antenna_scan();
        play_audio("Stop");

        function = 0;
        break;

      case 3:
        // set_timer(300);
        play_audio("Registering a new object");
        play_audio("Say the name of the object");
        // get object name
        get_audio(&objname);
        Serial.println();
        Serial.print("Output string>>");
        Serial.print(objname);
        Serial.println("<<");
        if (objname != "") {
          db_insert_object(&objname);
          play_audio("Bring the object closer to the antenna");
          antenna_scan_once();
          Serial.print("Code0: ");
          Serial.println(codes[0]);
          db_insert_code(&objname, codes[0]);
        }
        function = 0;
        // sound_error();
        break;
      case 4:
        // volume updown
        volume_lower /= 2;
        delay(100);
        play_audio("Lowering volume");
        Serial.print("Volume: ");
        Serial.println(volume_lower);
        function = 0;
        break;
      case 5:
        break;
      case 6: break;
      case 7: break;
      case 8: break;
      case 9: break;
      default:
        break;
    }
  }
}
