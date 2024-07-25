
const char *tail;
int rc;
int rec_count = 0;
void init_webserver() {
  server.on("/", []() {
    handleRoot(NULL, NULL);
  });
  server.on("/exec_sql", []() {
    String db_name = server.arg("db_name");
    String sql = server.arg("sql");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    handleRoot(db_name.c_str(), sql.c_str());
    //open_sql();
    /*if (openDb(db_name.c_str(),&db1)) {
      String resp = "Error opening database: ";
      resp += sqlite3_errmsg(db1);
      resp += "<pre>";
      resp += server.arg("db_name");
      resp += "</pre>";
      resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
      server.sendContent(resp);
      return;
    }
    */
    Serial.println(sqlite3_errmsg(db1));
    rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
    if (rc != SQLITE_OK) {
      String resp = "Failed to fetch data: ";
      resp += sqlite3_errmsg(db1);
      resp += "<br><br><a href='/'>back</a>";
      server.sendContent(resp);
      Serial.println(resp.c_str());
      return;
    }

    rec_count = 0;
    String resp = "<h2>Result:</h2><h3>";
    resp += sql;
    resp += "</h3><table cellspacing='1' cellpadding='1' border='1'>";
    server.sendContent(resp);
    bool first = true;
    while (sqlite3_step(res) == SQLITE_ROW) {
      resp = "";
      if (first) {
        int count = sqlite3_column_count(res);
        if (count == 0) {
          resp += "<tr><td>Statement executed successfully</td></tr>";
          rec_count = sqlite3_changes(db1);
          break;
        }
        resp += "<tr>";
        for (int i = 0; i < count; i++) {
          resp += "<td>";
          resp += sqlite3_column_name(res, i);
          resp += "</td>";
        }
        resp += "</tr>";
        first = false;
      }
      int count = sqlite3_column_count(res);
      resp += "<tr>";
      for (int i = 0; i < count; i++) {
        resp += "<td>";
        resp += (const char *)sqlite3_column_text(res, i);
        resp += "</td>";
      }
      resp += "</tr>";
      server.sendContent(resp);
      rec_count++;
    }
    resp = "</table><br><br>Number of records: ";
    resp += rec_count;
    resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
    server.sendContent(resp);
    sqlite3_finalize(res);

  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot(const char *db_name, const char *sql) {

  String temp;
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  temp = "<html><head>\
      <title>ESP32 Sqlite Web Console</title>\
      <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: large; Color: #000088; }\
      </style>\
  </head>\
  <body>\
      <h2>ESP32 Sqlite Web Console</h2>\
      <p>Uptime: ";
  temp += hr;
  temp += ":";
  temp += min % 60;
  temp += ":";
  temp += sec % 60;
  temp += "</p>\
      <form name='params' method='POST' action='exec_sql'>\
      <textarea style='font-size: medium; width:100%' rows='4' placeholder='Enter SQL Statement' name='sql'>";
  if (sql != NULL)
    temp += sql;
  temp += "</textarea> \
      <br>File name (prefix with /spiffs/ or /sd/ or /sdcard/):<br/><input type=text size='50' style='font-size: small' value='";
  if (db_name != NULL)
    temp += db_name;
  temp += "' name='db_name'/> \
      <br><br><input type=submit style='font-size: large' value='Execute'/>\
      </form><hr/>";

  server.send(200, "text/html", temp.c_str());
}

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}
