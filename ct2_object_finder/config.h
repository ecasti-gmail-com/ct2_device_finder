// WiFi network name and password:
const char* ssidName = "<YOUR_SSID>";
const char* ssidPswd = "<YOUR_PWD>";

#define URL_STT "http://nginx.192.168.2.31.nip.io/whisper/asr?output=json&language=en"
#define URL_SERVER "nginx.192.168.2.31.nip.io"
#define URL_PATH "/whisper/asr?output=json&language=en"
#define TTS_PATH1 "/marytts/process?INPUT_TEXT="
#define TTS_PATH2 "&INPUT_TYPE=TEXT&OUTPUT_TYPE=AUDIO&AUDIO=WAVE_FILE&LOCALE=en_US"
#define BUTTON_VOICE 2
#define BUTTON_IDENTIFY 1
#define BUTTON_FIND 3

/*
#define BUTTONVP 5
#define BUTTONVM 6
#define BUTTON_ADD 4
*/