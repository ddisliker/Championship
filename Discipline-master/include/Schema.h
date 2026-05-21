#pragma once
#include "Types.h"
#include <vector>
#include <string>
#include <stdexcept>

struct ColumnDef {
    std::string name;
    DataType    type       = DataType::VARCHAR;
    int         varcharSize= 255;
    bool        primaryKey = false;
    bool        notNull    = false;
    bool        unique     = false;
    // Foreign key
    std::string fkTable;
    std::string fkColumn;

    std::string serialize() const {
        // format: name|type|varcharSize|pk|nn|uq|fkTable|fkCol
        return name + "|" + typeToString(type) + "|" + std::to_string(varcharSize)
            + "|" + (primaryKey?"1":"0")
            + "|" + (notNull?"1":"0")
            + "|" + (unique?"1":"0")
            + "|" + fkTable + "|" + fkColumn;
    }

    static ColumnDef deserialize(const std::string& line) {
        ColumnDef col;
        std::vector<std::string> parts;
        std::string cur;
        for (char c : line) {
            if (c == '|') { parts.push_back(cur); cur.clear(); }
            else cur += c;
        }
        parts.push_back(cur);
        if (parts.size() < 8) throw std::runtime_error("Bad schema line: " + line);
        col.name        = parts[0];
        col.type        = typeFromString(parts[1]);
        col.varcharSize = std::stoi(parts[2]);
        col.primaryKey  = parts[3] == "1";
        col.notNull     = parts[4] == "1";
        col.unique      = parts[5] == "1";
        col.fkTable     = parts[6];
        col.fkColumn    = parts[7];
        return col;
    }
};

struct Schema {
    std::string tableName;
    std::vector<ColumnDef> columns;

    int getColumnIndex(const std::string& colName) const {
        std::string lo = colName;
        for (auto& c : lo) c = (char)tolower(c);
        for (int i = 0; i < (int)columns.size(); i++) {
            std::string cn = columns[i].name;
            for (auto& c : cn) c = (char)tolower(c);
            if (cn == lo) return i;
        }
        return -1;
    }

    const ColumnDef& getColumn(const std::string& colName) const {
        int idx = getColumnIndex(colName);
        if (idx < 0) throw std::runtime_error("Column not found: " + colName);
        return columns[idx];
    }

    std::string serialize() const {
        std::string out = tableName + "\n";
        for (auto& col : columns) out += col.serialize() + "\n";
        return out;
    }

    static Schema deserialize(const std::string& text) {
        Schema s;
        std::istringstream ss(text);
        std::string line;
        bool first = true;
        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            if (first) { s.tableName = line; first = false; }
            else s.columns.push_back(ColumnDef::deserialize(line));
        }
        return s;
    }
};
