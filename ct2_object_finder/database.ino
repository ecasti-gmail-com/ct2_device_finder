/*
Database: contest_rfid
Table: objects

- obname (text)
- variants (text)

Table: code
- code (text)
- obid (numeric) -> objects.id ( no constraint)
*/

const char *data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
  int i;
  Serial.printf("%s: ", (const char *)data);
  for (i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.printf("\n");

  return 0;
}

int openDb(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}
/*
void open_sql() {
  if (openDb("/sd/ctrfid.db", &db1)) {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("setupSDCard PASS . SIZE = ");
    Serial.print(cardSize);
    Serial.println(" MB");
  } else {
    Serial.println("No valid SD Card dedected");
  }
}
void close_sql(){
  sqlite3_close(db1);
}
*/
char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {

  Serial.println(sql);
  long start = micros();
  int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    Serial.println(sqlite3_errmsg(db1));
  } else {
    Serial.printf("Operation done successfully\n");
  }
  Serial.print(F("Time taken:"));
  Serial.println(micros() - start);

  return rc;
}


void db_insert_object(String *obname) {
  char stringa[256];
  // char tmpstr[18];
  Serial.print("Output string>>");
  Serial.print(*obname);
  Serial.println("<<");
  int obid = db_get_object_id(obname);
  if (obid == 0) {
    sprintf(stringa, "Insert into objects(obname) values (\'%s\')", *obname);
    Serial.println();
    Serial.println(stringa);
    int rc = db_exec(db1, stringa);
    Serial.print("RC: ");
    Serial.println(rc);
    if (rc != SQLITE_OK) {
      Serial.println("Unable to store data");
    }
  } else {
    Serial.println("Object already on db");
  }
}
/*
Check if object already exists

*/
int db_get_object_id(String *obname) {
  int obid = 0;
  sqlite3_stmt *stmt;
  char stringa[128];
  sprintf(stringa, "Select count(*) from objects where obname = \'%s\'", *obname);
  Serial.println();
  Serial.println(stringa);
  int rc = sqlite3_prepare_v2(db1, stringa, 1000, &stmt, NULL);
  Serial.print("RC: ");
  Serial.println(rc);
  if (rc != SQLITE_OK) {
    Serial.println("Unable to read the data");
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    obid = sqlite3_column_int(stmt, 0);
  }
  Serial.println();
  Serial.print("obid: ");
  Serial.println(obid);
  sqlite3_finalize(stmt);

  return obid;
}
/*
Check if code already in use

*/
int db_get_code_id(String obcode) {
  int codeid = 0;
  sqlite3_stmt *stmt;
  char stringa[128];
  char tmpstr[30];
  obcode.toCharArray(tmpstr, obcode.length() + 1);

  sprintf(stringa, "Select count(*) from codes where code = \'%s\'", tmpstr);
  Serial.println(stringa);
  int rc = sqlite3_prepare_v2(db1, stringa, 1000, &stmt, NULL);
  Serial.print("RC: ");
  Serial.println(rc);
  if (rc != SQLITE_OK) {
    Serial.println("Unable to load data");
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    Serial.println("getting record");
    codeid = sqlite3_column_int(stmt, 0);
  }
  Serial.print("Codeid: ");
  Serial.println(codeid);
  sqlite3_finalize(stmt);

  return codeid;
}

bool db_get_object_name(String obcode, bool play = 0) {
  int codeid = 0;
  bool found = false;
  sqlite3_stmt *stmt;
  char stringa[256];
  char tmpstr[120];
 // *obcode->toCharArray(tmpstr, *obcode->length() + 1);
  obcode.toCharArray(tmpstr, obcode.length() + 1);
  sprintf(stringa, "Select obname from codes where code = \'%s\'", tmpstr);
  Serial.println(stringa);
  int rc = sqlite3_prepare_v2(db1, stringa, 1000, &stmt, NULL);
  Serial.print("RC: ");
  Serial.println(rc);
  if (rc != SQLITE_OK) {
    Serial.println("Unable to load data");
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    found = true;
    // tmpstr = sqlite3_column_text(stmt, 0);
    sprintf(tmpstr, "%s", sqlite3_column_text(stmt, 0));
    if (play) {
      play_audio(tmpstr);
    }
    Serial.print("identified: ");
    Serial.println(tmpstr);
  }


  sqlite3_finalize(stmt);
  return found;
}

void db_insert_code(String *obname, String obcode) {
  char stringa[128];
  char tmpstr[30];
  obcode.trim();
  obcode.toCharArray(tmpstr, obcode.length() + 1);
  Serial.print("obcode: ");
  Serial.println(obcode);
  Serial.print("tmpstr: ");
  Serial.println(tmpstr);
  int codeid = db_get_code_id(obcode);
  if (codeid == 0) {
    Serial.println("Code not present already");
    //  int obid = db_get_object_id(obname);

    sprintf(stringa, "Insert into codes(obname,code) values (\'%s\',\'%s\')", *obname, tmpstr);
    int rc = db_exec(db1, stringa);
    Serial.print("RC: ");
    Serial.println(rc);
    if (rc != SQLITE_OK) {
      Serial.println("Unable to store data");
    }
  } else {
    play_audio("This code is already in use");
  }
}
void db_search_codes(String *obname) {
  sqlite3_stmt *stmt;
  codes_p = 0;
  char tmpstr[128];
  char stringa[256];
  //int obid = db_get_object_id(obname);
  sprintf(stringa, "Select code from codes where obname  = \'%s\';", *obname);
  Serial.println(stringa);
  int rc = sqlite3_prepare_v2(db1, stringa, 1000, &stmt, NULL);
  Serial.print("RC: ");
  Serial.println(rc);
  if (rc != SQLITE_OK) {
    Serial.println("Unable to read the data");
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    // Serial.println(sqlite3_column_text(stmt, 0));
    sprintf(tmpstr, "%s", sqlite3_column_text(stmt, 0));
    Serial.println(tmpstr);
    codes[codes_p++] = tmpstr;
  }
  Serial.print("Found objects: ");
  Serial.println(codes_p);
  sqlite3_finalize(stmt);
  return;
}