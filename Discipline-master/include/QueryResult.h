#pragma once
#include "Types.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

struct QueryResult {
    bool success = true;
    std::string message;
    // For SELECT results
    std::vector<std::string>         columns;
    std::vector<std::vector<Value>>  rows;
    int64_t affectedRows = 0;

    static QueryResult ok(const std::string& msg = "", int64_t affected = 0) {
        return { true, msg, {}, {}, affected };
    }

    static QueryResult error(const std::string& msg) {
        return { false, msg, {}, {}, 0 };
    }

    std::string toString() const {
        if (!success) return "ERROR: " + message;
        if (!columns.empty()) {
            // Calculate column widths
            std::vector<size_t> widths(columns.size());
            for (size_t i = 0; i < columns.size(); i++)
                widths[i] = columns[i].size();
            for (auto& row : rows)
                for (size_t i = 0; i < row.size() && i < widths.size(); i++)
                    widths[i] = std::max(widths[i], valueToString(row[i]).size());

            // Separator
            std::string sep = "+";
            for (auto w : widths) sep += std::string(w + 2, '-') + "+";
            sep += "\n";

            std::ostringstream out;
            out << sep;
            // Header
            out << "|";
            for (size_t i = 0; i < columns.size(); i++)
                out << " " << std::left << std::setw((int)widths[i]) << columns[i] << " |";
            out << "\n" << sep;
            // Rows
            for (auto& row : rows) {
                out << "|";
                for (size_t i = 0; i < row.size() && i < widths.size(); i++)
                    out << " " << std::left << std::setw((int)widths[i]) << valueToString(row[i]) << " |";
                out << "\n";
            }
            out << sep;
            out << rows.size() << " row(s) selected.\n";
            return out.str();
        }
        if (!message.empty()) return message;
        return "OK (" + std::to_string(affectedRows) + " rows affected)";
    }
};
