#include "database.h"
#include "message_handler.h"

Database* Database::instance = nullptr;
std::string dblabel = "DB";
static MessageHandler* messageHandlerDB;

Database::Database(const std::string& dbPath) : db(nullptr), dbPath(dbPath) {}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

void Database::dbPrint(std::string msg) {
    if (messageHandlerDB != nullptr) {
        messageHandlerDB->sendDebug("[" + dblabel + "] " + msg);
    }
}

void Database::giveMsgHandler(MessageHandler* handler) {
    messageHandlerDB = handler;
}

bool Database::init() {
    if (sqlite3_open(dbPath.c_str(), &db)) {
        dbPrint("Cannot open database: " + std::string(sqlite3_errmsg(db)));
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
        dbPrint("SQL error: " + std::string(errMsg));
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
    dbPrint("Trying to save (database) ICAO: " + icao + ", Timestamp: " + timeStr + ", Lat: " + lat + ", Lon: " + lon);
    std::string query = "INSERT OR REPLACE INTO AircraftData (icao, timestamp, latitude, longitude) VALUES ('" + icao + "', '" + timeStr + "', '" + lat + "', '" + lon + "');";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        dbPrint("SQL error: " + std::string(errMsg));
        sqlite3_free(errMsg);
    }
}

void Database::debugLoadAllAircraftData() {
    const char* query = "SELECT icao, timestamp, latitude, longitude FROM AircraftData;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        dbPrint("Failed to fetch data: " + std::string(sqlite3_errmsg(db)));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string icao = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string latitude = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string longitude = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        dbPrint("ICAO: " + icao + ", Timestamp: " + timestamp + ", Lat: " + latitude + ", Lon: " + longitude);
    }

    sqlite3_finalize(stmt);
}

void Database::loadAllAircraftData(std::string& message) {
    const char* query = "SELECT icao, timestamp, latitude, longitude FROM AircraftData;";
    sqlite3_stmt* stmt;
    nlohmann::json handoverArray = nlohmann::json::array();

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        dbPrint("Failed to fetch data: " + std::string(sqlite3_errmsg(db)));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        nlohmann::json j; // struct to json for js handling
        std::string icao = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string latitude = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string longitude = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        j["icao"] = icao;
        j["timestamp"] = timestamp;
        j["latitude"] = latitude;
        j["longitude"] = longitude;
        handoverArray.push_back(j);
    }
    message = handoverArray.dump();
    sqlite3_finalize(stmt);
}

void Database::deleteAllAircraftData() {
    const char* query = "DELETE FROM AircraftData;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        dbPrint("Failed to prepare delete: " + std::string(sqlite3_errmsg(db)));
        return;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        dbPrint("Failed to execute delete: " + std::string(sqlite3_errmsg(db)));
    }
    else {
        dbPrint("All aircraft data deleted.");
    }

    sqlite3_finalize(stmt);
}

//update this later to return actual information on it, for now as long as return > 1, it's in the DB
int Database::findAircraftByICAO(const std::string& icaoToFind) {
    const char* query = "SELECT icao, timestamp, latitude, longitude FROM AircraftData WHERE icao = ?;";
    sqlite3_stmt* stmt;
    int rowCount = 0;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        dbPrint("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
        return 0;
    }

    // Bind the ICAO value to the first parameter in the query
    if (sqlite3_bind_text(stmt, 1, icaoToFind.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        dbPrint("Failed to bind ICAO: " + std::string(sqlite3_errmsg(db)));
        sqlite3_finalize(stmt);
        return 0;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string icao = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string latitude = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string longitude = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        dbPrint("ICAO: " + icao + ", Timestamp: " + timestamp + ", Lat: " + latitude + ", Lon: " + longitude);
        ++rowCount;
    }

    sqlite3_finalize(stmt);
    return rowCount;
}

int Database::deleteAircraftByICAO(const std::string& icaoToDelete) {
    const char* query = "DELETE FROM AircraftData WHERE icao = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        dbPrint("Failed to prepare DELETE statement: " + std::string(sqlite3_errmsg(db)));
        return 0;
    }

    // Bind the ICAO value to the query
    if (sqlite3_bind_text(stmt, 1, icaoToDelete.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        dbPrint("Failed to bind ICAO: " + std::string(sqlite3_errmsg(db)));
        sqlite3_finalize(stmt);
        return 0;
    }

    // Execute the DELETE query
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        dbPrint("Failed to delete row: " + std::string(sqlite3_errmsg(db)));
        sqlite3_finalize(stmt);
        return 0;
    }

    int rowsDeleted = sqlite3_changes(db);
    dbPrint("Rows deleted: " + std::to_string(rowsDeleted));

    sqlite3_finalize(stmt);
    return 1; // deletion worked
}





Database& Database::getInstance(const std::string& dbPath) {
    if (instance == nullptr) {
        instance = new Database(dbPath);
        if (!instance->init()) {
            delete instance;
            instance = nullptr;
        }
    }
    //dbPrint("getting database instance");
    //TODO: save as ref in relevant part (map javascript)
    return *instance;
}
