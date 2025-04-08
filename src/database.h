#include "sqlite3.h"
#include <string>
#include <iostream>
#include <ctime>
#include "message_handler.h"

class Database {
public:
    Database(const std::string& dbPath);
    ~Database();

    static void dbPrint(std::string msg);
    static void giveMsgHandler(MessageHandler* handler);

    bool init();

    void saveAircraftData(const std::string& icao, const std::string& lat, const std::string& lon);
    void loadAllAircraftData();

    static Database& getInstance(const std::string& dbPath = "");

private:
    sqlite3* db;
    std::string dbPath;
    static Database* instance;
};