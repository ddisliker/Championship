#pragma once
#include "Table.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <stdexcept>

namespace fs = std::filesystem;

// Singleton pattern for storage management
class Storage {
public:
    static Storage& instance() {
        static Storage inst;
        return inst;
    }

    void setDataRoot(const std::string& path) { dataRoot = path; }

    // ---- Database ----
    bool databaseExists(const std::string& db) const {
        return fs::exists(dbPath(db));
    }

    void createDatabase(const std::string& db) {
        fs::create_directories(dbPath(db));
    }

    void dropDatabase(const std::string& db) {
        if (fs::exists(dbPath(db)))
            fs::remove_all(dbPath(db));
    }

    std::vector<std::string> listDatabases() const {
        std::vector<std::string> result;
        if (!fs::exists(dataRoot)) return result;
        for (auto& e : fs::directory_iterator(dataRoot))
            if (e.is_directory()) result.push_back(e.path().filename().string());
        return result;
    }

    // ---- Table ----
    void saveSchema(const std::string& db, const Schema& schema) {
        ensureDb(db);
        std::ofstream f(schemaPath(db, schema.tableName));
        f << schema.serialize();
    }

    Schema loadSchema(const std::string& db, const std::string& tableName) {
        std::ifstream f(schemaPath(db, tableName));
        if (!f.is_open()) throw std::runtime_error("Table not found: " + tableName);
        std::ostringstream ss; ss << f.rdbuf();
        return Schema::deserialize(ss.str());
    }

    void saveRows(const std::string& db, const std::string& tableName,
                  const Schema& schema, const std::vector<Row>& rows)
    {
        ensureDb(db);
        std::ofstream f(dataPath(db, tableName));
        for (auto& row : rows) {
            for (size_t i = 0; i < row.size(); i++) {
                if (i) f << "\x1F"; // ASCII unit separator as delimiter
                std::string s = valueToString(row[i]);
                // Escape the separator char in data
                for (char c : s) {
                    if (c == '\x1F') f << "\\x1F";
                    else if (c == '\n') f << "\\n";
                    else if (c == '\\') f << "\\\\";
                    else f << c;
                }
            }
            f << "\n";
        }
    }

    std::vector<Row> loadRows(const std::string& db, const std::string& tableName,
                              const Schema& schema)
    {
        std::vector<Row> rows;
        std::ifstream f(dataPath(db, tableName));
        if (!f.is_open()) return rows;
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            Row row;
            std::string cell;
            for (size_t i = 0; i < line.size(); i++) {
                char c = line[i];
                if (c == '\x1F') {
                    row.push_back(decodeCell(cell, schema, (int)row.size()));
                    cell.clear();
                } else if (c == '\\' && i+1 < line.size()) {
                    char nc = line[++i];
                    if (nc == 'x' && i+3 < line.size() && line[i+1]=='1' && line[i+2]=='F') {
                        cell += '\x1F'; i += 2;
                    } else if (nc == 'n') cell += '\n';
                    else if (nc == '\\') cell += '\\';
                    else { cell += c; cell += nc; }
                } else cell += c;
            }
            row.push_back(decodeCell(cell, schema, (int)row.size()));
            rows.push_back(row);
        }
        return rows;
    }

    void dropTable(const std::string& db, const std::string& tableName) {
        auto sp = schemaPath(db, tableName);
        auto dp = dataPath(db, tableName);
        if (fs::exists(sp)) fs::remove(sp);
        if (fs::exists(dp)) fs::remove(dp);
    }

    bool tableExists(const std::string& db, const std::string& tableName) const {
        return fs::exists(schemaPath(db, tableName));
    }

    std::vector<std::string> listTables(const std::string& db) const {
        std::vector<std::string> result;
        if (!fs::exists(dbPath(db))) return result;
        for (auto& e : fs::directory_iterator(dbPath(db)))
            if (e.path().extension() == ".schema")
                result.push_back(e.path().stem().string());
        return result;
    }

private:
    std::string dataRoot = "./data";

    fs::path dbPath(const std::string& db) const { return fs::path(dataRoot) / db; }
    fs::path schemaPath(const std::string& db, const std::string& t) const {
        return dbPath(db) / (t + ".schema");
    }
    fs::path dataPath(const std::string& db, const std::string& t) const {
        return dbPath(db) / (t + ".dat");
    }

    void ensureDb(const std::string& db) {
        fs::create_directories(dbPath(db));
    }

    Value decodeCell(const std::string& cell, const Schema& schema, int colIdx) const {
        if (colIdx >= (int)schema.columns.size()) return NullValue{};
        return stringToValue(cell, schema.columns[colIdx].type);
    }

    Storage() = default;
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
};
