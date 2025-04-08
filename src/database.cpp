#include "database.h"
#include "src/message_handler.h"

Database* Database::instance = nullptr;
std::string dblabel = "DB";
MessageHandler* messageHandler;

Database::Database(const std::string& dbPath) : db(nullptr), dbPath(dbPath) {}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

void labelPrint(std::string msg) {
    if (messageHandler != nullptr) {
        messageHandler->sendDebug("[" + dblabel + "] " + msg);
    }
}

bool Database::init() {
    if (sqlite3_open(dbPath.c_str(), &db)) {
        labelPrint("Cannot open database: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    //create it if it does not exist
    const char* createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS AircraftData (
            icao TEXT PRIMARY KEY,
            timestamp TEXT,
            latitude TEXT,
            longitude TEXT
        );
    )";
    
    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTableQuery, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        labelPrint("SQL error: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

void Database::saveAircraftData(const std::string& icao, const std::string& lat, const std::string& lon) {
    time_t now = time(0);
    struct tm tstruct;
    char timeStr[80];
    localtime_s(&tstruct, &now);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %X", &tstruct);
    
    std::string query = "INSERT OR REPLACE INTO AircraftData (icao, timestamp, latitude, longitude) VALUES ('" 
                        + icao + "', '" + timeStr + "', '" + lat + "', '" + lon + "');";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        labelPrint("SQL error: " + std::string(errMsg));
        sqlite3_free(errMsg);
    }
}

void Database::loadAllAircraftData() {
    const char* query = "SELECT icao, timestamp, latitude, longitude FROM AircraftData;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        labelPrint("Failed to fetch data: " + std::string(sqlite3_errmsg(db)));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string icao = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string latitude = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string longitude = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        labelPrint("ICAO: " + icao + ", Timestamp: " + timestamp + ", Lat: " + latitude + ", Lon: " + longitude);
    }

    sqlite3_finalize(stmt);
}

Database& Database::getInstance(const std::string& dbPath) {
    if (instance == nullptr) {
        instance = new Database(dbPath);
        if (!instance->init()) {
            delete instance;
            instance = nullptr;
        }
    }
    return *instance;
}
