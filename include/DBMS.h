#pragma once
/**
 * DBMS – главный класс системы управления базой данных.
 * Реализует паттерн проектирования "Фасад" (Facade Pattern):
 * скрывает сложность подсистем (Parser, Storage, Table)
 * за единым простым интерфейсом.
 */

#include "Parser.h"
#include "Storage.h"
#include "Table.h"
#include "QueryResult.h"
#include <unordered_map>
#include <memory>
#include <functional>

class DBMS {
public:
    explicit DBMS(const std::string& dataPath = "./data") {
        Storage::instance().setDataRoot(dataPath);
    }

    // Execute a raw SQL string (may contain multiple statements separated by ;)
    std::vector<QueryResult> execute(const std::string& sql) {
        std::vector<QueryResult> results;
        // Split by semicolon
        std::vector<std::string> stmts = splitStatements(sql);
        for (auto& stmt : stmts) {
            auto r = executeOne(stmt);
            results.push_back(r);
        }
        return results;
    }

    // Execute single statement
    QueryResult executeOne(const std::string& sql) {
        std::string trimmed = trim(sql);
        if (trimmed.empty()) return QueryResult::ok();
        try {
            Parser p(trimmed);
            Statement stmt = p.parse();
            return std::visit([this](auto&& s) { return handle(s); }, stmt);
        } catch (const std::exception& e) {
            return QueryResult::error(e.what());
        }
    }

    std::string currentDb() const { return activeDb; }

private:
    std::string activeDb;
    // Cache of loaded tables
    std::unordered_map<std::string, std::shared_ptr<Table>> tableCache;

    // ---- Handlers ----

    QueryResult handle(const CreateDbStmt& s) {
        if (Storage::instance().databaseExists(s.dbName)) {
            if (s.ifNotExists) return QueryResult::ok("Database already exists (skipped)");
            return QueryResult::error("Database already exists: " + s.dbName);
        }
        Storage::instance().createDatabase(s.dbName);
        return QueryResult::ok("Database created: " + s.dbName);
    }

    QueryResult handle(const DropDbStmt& s) {
        if (!Storage::instance().databaseExists(s.dbName))
            return QueryResult::error("Database not found: " + s.dbName);
        // Evict cache
        tableCache.clear();
        if (activeDb == s.dbName) activeDb.clear();
        Storage::instance().dropDatabase(s.dbName);
        return QueryResult::ok("Database dropped: " + s.dbName);
    }

    QueryResult handle(const UseDatabaseStmt& s) {
        if (!Storage::instance().databaseExists(s.dbName))
            return QueryResult::error("Database not found: " + s.dbName);
        activeDb = s.dbName;
        tableCache.clear();
        return QueryResult::ok("Using database: " + s.dbName);
    }

    QueryResult handle(const ShowDbsStmt&) {
        auto dbs = Storage::instance().listDatabases();
        QueryResult r; r.success = true;
        r.columns = {"database"};
        for (auto& d : dbs) r.rows.push_back({Value{d}});
        return r;
    }

    QueryResult handle(const ShowTablesStmt&) {
        if (activeDb.empty()) return QueryResult::error("No database selected. Use: USE <dbname>");
        auto tables = Storage::instance().listTables(activeDb);
        QueryResult r; r.success = true;
        r.columns = {"table_name"};
        for (auto& t : tables) r.rows.push_back({Value{t}});
        return r;
    }

    QueryResult handle(const CreateTableStmt& s) {
        if (activeDb.empty()) return QueryResult::error("No database selected");
        if (Storage::instance().tableExists(activeDb, s.tableName)) {
            if (s.ifNotExists) return QueryResult::ok("Table already exists (skipped)");
            return QueryResult::error("Table already exists: " + s.tableName);
        }
        // Build schema
        Schema schema; schema.tableName = s.tableName;
        for (auto& cs : s.columns) {
            ColumnDef col;
            col.name = cs.name;
            col.type = typeFromString(cs.typeName);
            col.varcharSize = cs.typeSize;
            col.primaryKey  = cs.primaryKey;
            col.notNull     = cs.notNull;
            col.unique      = cs.unique;
            col.fkTable     = cs.fkTable;
            col.fkColumn    = cs.fkColumn;
            schema.columns.push_back(col);
        }
        Storage::instance().saveSchema(activeDb, schema);
        // Create empty data file
        Storage::instance().saveRows(activeDb, s.tableName, schema, {});
        return QueryResult::ok("Table created: " + s.tableName);
    }

    QueryResult handle(const DropTableStmt& s) {
        if (activeDb.empty()) return QueryResult::error("No database selected");
        if (!Storage::instance().tableExists(activeDb, s.tableName)) {
            if (s.ifExists) return QueryResult::ok("Table not found (skipped)");
            return QueryResult::error("Table not found: " + s.tableName);
        }
        tableCache.erase(s.tableName);
        Storage::instance().dropTable(activeDb, s.tableName);
        return QueryResult::ok("Table dropped: " + s.tableName);
    }

    QueryResult handle(const SelectStmt& s) {
        if (activeDb.empty()) return QueryResult::error("No database selected");
        auto tbl = loadTable(s.tableName);
        if (!tbl) return QueryResult::error("Table not found: " + s.tableName);
        auto cond = buildCondition(s.hasWhere, s.where);
        return tbl->select(s.projection, cond);
    }

    QueryResult handle(const InsertStmt& s) {
        if (activeDb.empty()) return QueryResult::error("No database selected");
        auto tbl = loadTable(s.tableName);
        if (!tbl) return QueryResult::error("Table not found: " + s.tableName);
        int64_t total = 0;
        for (auto& vals : s.valueSets) {
            auto r = tbl->insert(s.columns, vals);
            if (!r.success) return r;
            total += r.affectedRows;
        }
        saveTable(*tbl);
        return QueryResult::ok("", total);
    }

    QueryResult handle(const UpdateStmt& s) {
        if (activeDb.empty()) return QueryResult::error("No database selected");
        auto tbl = loadTable(s.tableName);
        if (!tbl) return QueryResult::error("Table not found: " + s.tableName);
        std::vector<std::pair<std::string,std::string>> assignments;
        for (auto& a : s.assignments) assignments.emplace_back(a.column, a.value);
        auto cond = buildCondition(s.hasWhere, s.where);
        auto r = tbl->update(assignments, cond);
        if (r.success) saveTable(*tbl);
        return r;
    }

    QueryResult handle(const DeleteStmt& s) {
        if (activeDb.empty()) return QueryResult::error("No database selected");
        auto tbl = loadTable(s.tableName);
        if (!tbl) return QueryResult::error("Table not found: " + s.tableName);
        auto cond = buildCondition(s.hasWhere, s.where);
        auto r = tbl->deleteRows(cond);
        if (r.success) saveTable(*tbl);
        return r;
    }

    // ---- Helpers ----

    std::shared_ptr<Table> loadTable(const std::string& name) {
        auto it = tableCache.find(name);
        if (it != tableCache.end()) return it->second;
        if (!Storage::instance().tableExists(activeDb, name)) return nullptr;
        Schema schema = Storage::instance().loadSchema(activeDb, name);
        auto rows = Storage::instance().loadRows(activeDb, name, schema);
        auto tbl = std::make_shared<Table>(schema);
        tbl->rows = std::move(rows);
        tableCache[name] = tbl;
        return tbl;
    }

    void saveTable(const Table& tbl) {
        Storage::instance().saveRows(activeDb, tbl.schema.tableName, tbl.schema, tbl.rows);
    }

    // Build a RowFilter lambda from AST
    RowFilter buildCondition(bool hasWhere, const WhereCondition& where) {
        if (!hasWhere) return nullptr;
        return [where](const Row& row, const Schema& schema) -> bool {
            if (where.conditions.empty()) return true;
            bool result = evalSimple(where.conditions[0], row, schema);
            for (size_t i = 0; i < where.ops.size(); i++) {
                bool next = evalSimple(where.conditions[i+1], row, schema);
                if (where.ops[i] == LogicOp::AND) result = result && next;
                else result = result || next;
            }
            return result;
        };
    }

    static bool evalSimple(const SimpleCondition& c, const Row& row, const Schema& schema) {
        int idx = schema.getColumnIndex(c.column);
        if (idx < 0 || idx >= (int)row.size()) return false;
        const Value& lhs = row[idx];

        Value rhs;
        if (c.valueIsColumn) {
            int ridx = schema.getColumnIndex(c.value);
            if (ridx < 0 || ridx >= (int)row.size()) return false;
            rhs = row[ridx];
        } else {
            rhs = stringToValue(c.value, schema.columns[idx].type);
        }

        // NULL comparisons
        bool lNull = std::holds_alternative<NullValue>(lhs);
        bool rNull = std::holds_alternative<NullValue>(rhs);
        if (lNull || rNull) {
            if (c.op == CondOp::EQ) return lNull && rNull;
            if (c.op == CondOp::NEQ) return !(lNull && rNull);
            return false;
        }

        switch (c.op) {
            case CondOp::EQ:  return compareValues(lhs, rhs) == 0;
            case CondOp::NEQ: return compareValues(lhs, rhs) != 0;
            case CondOp::LT:  return compareValues(lhs, rhs) <  0;
            case CondOp::GT:  return compareValues(lhs, rhs) >  0;
            case CondOp::LEQ: return compareValues(lhs, rhs) <= 0;
            case CondOp::GEQ: return compareValues(lhs, rhs) >= 0;
        }
        return false;
    }

    static int compareValues(const Value& a, const Value& b) {
        // Normalize to double for numeric comparison
        auto toDouble = [](const Value& v) -> double {
            if (auto p = std::get_if<int64_t>(&v)) return (double)*p;
            if (auto p = std::get_if<double>(&v)) return *p;
            if (auto p = std::get_if<bool>(&v)) return *p ? 1.0 : 0.0;
            return 0.0;
        };
        if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
            auto sa = std::get<std::string>(a), sb = std::get<std::string>(b);
            if (sa < sb) return -1; if (sa > sb) return 1; return 0;
        }
        double da = toDouble(a), db = toDouble(b);
        if (da < db) return -1; if (da > db) return 1; return 0;
    }

    static std::string trim(const std::string& s) {
        size_t l = s.find_first_not_of(" \t\r\n");
        if (l == std::string::npos) return "";
        size_t r = s.find_last_not_of(" \t\r\n ;");
        return s.substr(l, r - l + 1);
    }

    static std::vector<std::string> splitStatements(const std::string& sql) {
        std::vector<std::string> result;
        std::string cur;
        bool inStr = false;
        char strChar = 0;
        for (char c : sql) {
            if (!inStr && (c == '\'' || c == '"')) { inStr = true; strChar = c; cur += c; }
            else if (inStr && c == strChar) { inStr = false; cur += c; }
            else if (!inStr && c == ';') {
                auto t = trim(cur);
                if (!t.empty()) result.push_back(t);
                cur.clear();
            } else cur += c;
        }
        auto t = trim(cur);
        if (!t.empty()) result.push_back(t);
        return result;
    }
};
