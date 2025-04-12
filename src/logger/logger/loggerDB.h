#ifndef LOGGERDB_H
#define LOGGERDB_H

#include "logger.h"

void Logger::createLaserSetTable()
{
    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS laser_driver_setting (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp SQLITE_INT64_TYPE,
            level TEXT,
            message TEXT,
            turn_on INTEGER,
            importance INTEGER,
            baud_rate INTEGER,
            filepath TEXT,
            current REAL,
            current_max REAL,
            tec_temperature REAL,
            tec_temperature_max REAL,
            tec_temperature_min REAL,
            tec_current_limit REAL,
            sensor_temperature_min REAL,
            sensor_temperature_max REAL,
            frequency REAL,
            duration REAL
        );
    )";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::string error = "Error creating LaserSet table: " + std::string(errMsg);
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }
}

void Logger::createTable()
{
    const char* createSourceTableSQL = R"(
        CREATE TABLE IF NOT EXISTS source (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE,
            description TEXT
        );
    )";

    const char* createMessageTypeTableSQL = R"(
        CREATE TABLE IF NOT EXISTS message_type (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE,
            description TEXT
        );
    )";

    const char* createLogGeneralTableSQL = R"(
        CREATE TABLE IF NOT EXISTS log_general (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp SQLITE_INT64_TYPE,
            severity TEXT,
            source_id INTEGER,
            message_type_id INTEGER,
            message TEXT,
            FOREIGN KEY(source_id) REFERENCES source(id),
            FOREIGN KEY(message_type_id) REFERENCES message_type(id)
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, createSourceTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::string error = "Error creating source table: " + std::string(errMsg);
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }

    if (sqlite3_exec(db, createMessageTypeTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::string error = "Error creating message_type table: " + std::string(errMsg);
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }

    if (sqlite3_exec(db, createLogGeneralTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::string error = "Error creating log_general table: " + std::string(errMsg);
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }
}

int Logger::getOrInsertSourceId(const std::string& sourceName)
{
    sqlite3_stmt* stmt;
    const char* selectSQL = "SELECT id FROM source WHERE name = ?;";
    if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) != SQLITE_OK)
    {
        throw std::runtime_error("Failed to prepare SQLite statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, sourceName.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return id;
    }
    sqlite3_finalize(stmt);

    const char* insertSQL = "INSERT INTO source (name, description) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK)
    {
        throw std::runtime_error("Failed to prepare SQLite statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, sourceName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, sourceName.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to insert source: " + std::string(sqlite3_errmsg(db)));
    }

    int id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return id;
}

int Logger::getOrInsertMessageTypeId(const std::string& messageTypeName)
{
    sqlite3_stmt* stmt;
    const char* selectSQL = "SELECT id FROM message_type WHERE name = ?;";
    if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) != SQLITE_OK)
    {
        throw std::runtime_error("Failed to prepare SQLite statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, messageTypeName.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return id;
    }
    sqlite3_finalize(stmt);

    const char* insertSQL = "INSERT INTO message_type (name, description) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK)
    {
        throw std::runtime_error("Failed to prepare SQLite statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, messageTypeName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, messageTypeName.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to insert message type: " + std::string(sqlite3_errmsg(db)));
    }

    int id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return id;
}

void Logger::insertIntoLaserSetTable(int64_t timestamp, const std::string& level, const std::string& command, const std::vector<std::string>& params)
{
    sqlite3_stmt* stmt;
    const char* insertSQL = R"(
        INSERT INTO laser_driver_setting (
            timestamp, level, message, turn_on, importance, baud_rate, filepath, current, current_max,
            tec_temperature, tec_temperature_max, tec_temperature_min, tec_current_limit,
            sensor_temperature_min, sensor_temperature_max, frequency, duration
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK)
    {
        throw std::runtime_error("Failed to prepare SQLite statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int64(stmt, 1, timestamp);
    sqlite3_bind_text(stmt, 2, level.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, command.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, std::stoi(params[0]));
    sqlite3_bind_int(stmt, 5, std::stoi(params[1]));
    sqlite3_bind_int(stmt, 6, std::stoi(params[2]));
    sqlite3_bind_text(stmt, 7, params[3].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 8, std::stod(params[4]));
    sqlite3_bind_double(stmt, 9, std::stod(params[5]));
    sqlite3_bind_double(stmt, 10, std::stod(params[6]));
    sqlite3_bind_double(stmt, 11, std::stod(params[7]));
    sqlite3_bind_double(stmt, 12, std::stod(params[8]));
    sqlite3_bind_double(stmt, 13, std::stod(params[9]));
    sqlite3_bind_double(stmt, 14, std::stod(params[10]));
    sqlite3_bind_double(stmt, 15, std::stod(params[11]));
    sqlite3_bind_double(stmt, 16, std::stod(params[12]));
    sqlite3_bind_double(stmt, 17, std::stod(params[13]));

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "SQLite insertion error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

void Logger::insertIntoTable(int64_t timestamp, const std::string& severity, const std::string& source, const std::string& messageType, const std::string& message) {
    // Запись в message_type
    int messageTypeId = getOrInsertMessageTypeId(messageType);

    // Запись в source
    int sourceId = getOrInsertSourceId(source);

    // Вставка в log_general
    sqlite3_stmt* stmt;
    const char* insertSQL = R"(
        INSERT INTO log_general (
            timestamp, severity, source_id, message_type_id, message
        ) VALUES (?, ?, ?, ?, ?);
    )";
    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare SQLite statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int64(stmt, 1, timestamp);
    sqlite3_bind_text(stmt, 2, severity.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, sourceId);  // Используем sourceId
    sqlite3_bind_int(stmt, 4, messageTypeId);  // Используем messageTypeId
    sqlite3_bind_text(stmt, 5, message.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "SQLite insertion error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

#endif
