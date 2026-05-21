#pragma once
#include <string>
#include <variant>
#include <stdexcept>
#include <sstream>
#include <iomanip>

enum class DataType { INTEGER, FLOAT, VARCHAR, BOOLEAN, NULL_TYPE };

// NULL sentinel
struct NullValue {};
inline bool operator==(const NullValue&, const NullValue&) { return true; }
inline bool operator<(const NullValue&, const NullValue&) { return false; }

using Value = std::variant<int64_t, double, std::string, bool, NullValue>;

inline std::string valueToString(const Value& v) {
    return std::visit([](auto&& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int64_t>)    return std::to_string(val);
        else if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss; oss << std::fixed << std::setprecision(6) << val; return oss.str();
        }
        else if constexpr (std::is_same_v<T, std::string>) return val;
        else if constexpr (std::is_same_v<T, bool>)   return val ? "true" : "false";
        else return "NULL";
    }, v);
}

inline Value stringToValue(const std::string& s, DataType type) {
    if (s == "NULL" || s == "null") return NullValue{};
    try {
        switch (type) {
            case DataType::INTEGER: return (int64_t)std::stoll(s);
            case DataType::FLOAT:   return std::stod(s);
            case DataType::VARCHAR: return s;
            case DataType::BOOLEAN: {
                std::string lo = s;
                for (auto& c : lo) c = (char)tolower(c);
                return lo == "true" || lo == "1";
            }
            default: return NullValue{};
        }
    } catch (...) {
        return NullValue{};
    }
}

inline std::string typeToString(DataType t) {
    switch (t) {
        case DataType::INTEGER: return "INTEGER";
        case DataType::FLOAT:   return "FLOAT";
        case DataType::VARCHAR: return "VARCHAR";
        case DataType::BOOLEAN: return "BOOLEAN";
        default: return "NULL";
    }
}

inline DataType typeFromString(const std::string& s) {
    std::string u = s;
    for (auto& c : u) c = (char)toupper(c);
    if (u == "INTEGER" || u == "INT") return DataType::INTEGER;
    if (u == "FLOAT" || u == "DOUBLE" || u == "REAL") return DataType::FLOAT;
    if (u.substr(0, 7) == "VARCHAR" || u == "TEXT" || u == "STRING") return DataType::VARCHAR;
    if (u == "BOOLEAN" || u == "BOOL") return DataType::BOOLEAN;
    throw std::runtime_error("Unknown type: " + s);
}
