//todo: replace with pragma twice
#pragma once

#include "sqlite3.h"
#include <string>
#include <iostream>
#include <ctime>
#include <src/json.h>

class MessageHandler; //forward declared so cmake doesn't go on a rampage

class Database {
public:
    Database(const std::string& dbPath);
    ~Database();

    static void dbPrint(std::string msg);
    static void giveMsgHandler(MessageHandler* handler);

    bool init();

    void saveAircraftData(const std::string& icao, const std::string& lat, const std::string& lon);
    void loadAllAircraftData(std::string& message);
    void debugLoadAllAircraftData();
    void deleteAllAircraftData();
    int findAircraftByICAO(const std::string& icaoToFind);

    static Database& getInstance(const std::string& dbPath = "");

private:
    sqlite3* db;
    std::string dbPath;
    static Database* instance;
};