#pragma once
#include "Schema.h"
#include "QueryResult.h"
#include <vector>
#include <functional>
#include <algorithm>

using Row = std::vector<Value>;

// Condition function: takes a row, returns bool
using RowFilter = std::function<bool(const Row&, const Schema&)>;

class Table {
public:
    Schema schema;
    std::vector<Row> rows;
    bool dirty = false; // needs saving

    explicit Table(Schema s) : schema(std::move(s)) {}

    // --- INSERT ---
    QueryResult insert(const std::vector<std::string>& colNames,
                       const std::vector<std::string>& valStrings)
    {
        Row row(schema.columns.size(), NullValue{});

        if (colNames.empty()) {
            // Positional
            if (valStrings.size() != schema.columns.size())
                return QueryResult::error("Value count does not match column count");
            for (size_t i = 0; i < schema.columns.size(); i++)
                row[i] = stringToValue(valStrings[i], schema.columns[i].type);
        } else {
            if (valStrings.size() != colNames.size())
                return QueryResult::error("Column/value count mismatch");
            for (size_t i = 0; i < colNames.size(); i++) {
                int idx = schema.getColumnIndex(colNames[i]);
                if (idx < 0) return QueryResult::error("Unknown column: " + colNames[i]);
                row[idx] = stringToValue(valStrings[i], schema.columns[idx].type);
            }
        }

        // Constraint checks
        for (auto err = checkConstraints(row, -1); !err.empty();)
            return QueryResult::error(err);

        rows.push_back(row);
        dirty = true;
        return QueryResult::ok("", 1);
    }

    // --- SELECT ---
    QueryResult select(const std::vector<std::string>& projection,
                       const RowFilter& cond) const
    {
        QueryResult res;
        res.success = true;

        // Resolve projection
        std::vector<int> colIdx;
        if (projection.size() == 1 && projection[0] == "*") {
            for (size_t i = 0; i < schema.columns.size(); i++) {
                colIdx.push_back((int)i);
                res.columns.push_back(schema.columns[i].name);
            }
        } else {
            for (auto& p : projection) {
                int idx = schema.getColumnIndex(p);
                if (idx < 0) return QueryResult::error("Unknown column: " + p);
                colIdx.push_back(idx);
                res.columns.push_back(schema.columns[idx].name);
            }
        }

        for (auto& row : rows) {
            if (!cond || cond(row, schema)) {
                Row projected;
                for (int i : colIdx) projected.push_back(row[i]);
                res.rows.push_back(projected);
            }
        }
        res.affectedRows = (int64_t)res.rows.size();
        return res;
    }

    // --- UPDATE ---
    QueryResult update(const std::vector<std::pair<std::string,std::string>>& assignments,
                       const RowFilter& cond)
    {
        int64_t count = 0;
        for (auto& row : rows) {
            if (!cond || cond(row, schema)) {
                Row newRow = row;
                for (auto& [colName, valStr] : assignments) {
                    int idx = schema.getColumnIndex(colName);
                    if (idx < 0) return QueryResult::error("Unknown column: " + colName);
                    newRow[idx] = stringToValue(valStr, schema.columns[idx].type);
                }
                auto err = checkConstraints(newRow, (int)(&row - &rows[0]));
                if (!err.empty()) return QueryResult::error(err);
                row = newRow;
                count++;
            }
        }
        dirty = true;
        return QueryResult::ok("", count);
    }

    // --- DELETE ---
    QueryResult deleteRows(const RowFilter& cond) {
        int64_t before = (int64_t)rows.size();
        rows.erase(
            std::remove_if(rows.begin(), rows.end(),
                [&](const Row& r){ return !cond || cond(r, schema); }),
            rows.end()
        );
        dirty = true;
        return QueryResult::ok("", before - (int64_t)rows.size());
    }

private:
    std::string checkConstraints(const Row& row, int skipIdx) {
        for (size_t i = 0; i < schema.columns.size(); i++) {
            auto& col = schema.columns[i];
            auto& val = row[i];
            bool isNull = std::holds_alternative<NullValue>(val);

            if (col.notNull && isNull)
                return "NOT NULL constraint failed on column: " + col.name;

            if ((col.primaryKey || col.unique) && !isNull) {
                for (int j = 0; j < (int)rows.size(); j++) {
                    if (j == skipIdx) continue;
                    if (rows[j][i] == val)
                        return (col.primaryKey ? "PRIMARY KEY" : "UNIQUE") +
                               std::string(" constraint failed on column: ") + col.name;
                }
            }
        }
        return "";
    }
};
