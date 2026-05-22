#include "../include/DBMS.h"
#include <cassert>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void cleanup() { if (fs::exists("./test_data")) fs::remove_all("./test_data"); }

int passed = 0, failed = 0;
#define TEST(name, expr) \
    do { \
        if (expr) { std::cout << "[PASS] " << name << "\n"; passed++; } \
        else { std::cout << "[FAIL] " << name << "\n"; failed++; } \
    } while(0)

int main() {
    cleanup();
    DBMS db("./test_data");

    std::cout << "\n=== MyDB Unit Tests ===\n\n";

    // DDL: Create database
    auto r = db.executeOne("CREATE DATABASE test");
    TEST("CREATE DATABASE", r.success);

    r = db.executeOne("CREATE DATABASE test");
    TEST("CREATE DATABASE duplicate error", !r.success);

    r = db.executeOne("CREATE DATABASE IF NOT EXISTS test");
    TEST("CREATE DATABASE IF NOT EXISTS", r.success);

    // USE
    r = db.executeOne("USE test");
    TEST("USE DATABASE", r.success);
    TEST("Active DB set", db.currentDb() == "test");

    // CREATE TABLE
    r = db.executeOne("CREATE TABLE users (id INTEGER PRIMARY KEY, name VARCHAR(50) NOT NULL, age INTEGER)");
    TEST("CREATE TABLE", r.success);

    r = db.executeOne("CREATE TABLE users (id INTEGER PRIMARY KEY)");
    TEST("CREATE TABLE duplicate error", !r.success);

    // SHOW
    r = db.executeOne("SHOW TABLES");
    TEST("SHOW TABLES", r.success && !r.rows.empty());

    // INSERT
    r = db.executeOne("INSERT INTO users VALUES (1, 'Вася', 16)");
    TEST("INSERT positional", r.success && r.affectedRows == 1);

    r = db.executeOne("INSERT INTO users (id, name, age) VALUES (2, 'Юля', 67)");
    TEST("INSERT with columns", r.success);

    r = db.executeOne("INSERT INTO users VALUES (3, 'Чарли', 31)");
    TEST("INSERT third row", r.success);

    // PRIMARY KEY violation
    r = db.executeOne("INSERT INTO users VALUES (1, 'Дубль', 20)");
    TEST("PRIMARY KEY violation", !r.success);

    // SELECT *
    r = db.executeOne("SELECT * FROM users");
    TEST("SELECT * rows count", r.success && r.rows.size() == 3);
    TEST("SELECT * columns count", r.columns.size() == 3);

    // SELECT with projection
    r = db.executeOne("SELECT name, age FROM users");
    TEST("SELECT projection", r.success && r.columns.size() == 2);

    // SELECT WHERE =
    r = db.executeOne("SELECT * FROM users WHERE id = 1");
    TEST("SELECT WHERE id=1", r.success && r.rows.size() == 1);
    TEST("SELECT WHERE correct name", valueToString(r.rows[0][1]) == "Вася");

    // SELECT WHERE >
    r = db.executeOne("SELECT * FROM users WHERE age > 20");
    TEST("SELECT WHERE age>20", r.success && r.rows.size() == 2);

    // SELECT WHERE AND
    r = db.executeOne("SELECT * FROM users WHERE age > 15 AND age < 40");
    TEST("SELECT WHERE AND", r.success && r.rows.size() == 2);

    // UPDATE
    r = db.executeOne("UPDATE users SET age = 21 WHERE id = 1");
    TEST("UPDATE", r.success && r.affectedRows == 1);

    r = db.executeOne("SELECT * FROM users WHERE id = 1");
    TEST("UPDATE verify", r.success && valueToString(r.rows[0][2]) == "21");

    // DELETE
    r = db.executeOne("DELETE FROM users WHERE id = 3");
    TEST("DELETE", r.success && r.affectedRows == 1);

    r = db.executeOne("SELECT * FROM users");
    TEST("DELETE verify", r.success && r.rows.size() == 2);

    // Persistence: reload
    {
        DBMS db2("./test_data");
        db2.executeOne("USE test");
        auto r2 = db2.executeOne("SELECT * FROM users");
        TEST("Persistence: rows loaded", r2.success && r2.rows.size() == 2);
    }

    // Multi-value INSERT
    r = db.executeOne("INSERT INTO users VALUES (10, 'Аня', 25), (11, 'Боря', 33)");
    TEST("Multi-value INSERT", r.success && r.affectedRows == 2);

    // DROP TABLE
    r = db.executeOne("DROP TABLE users");
    TEST("DROP TABLE", r.success);
    r = db.executeOne("SELECT * FROM users");
    TEST("SELECT on dropped table fails", !r.success);

    // DROP DATABASE
    r = db.executeOne("DROP DATABASE test");
    TEST("DROP DATABASE", r.success);

    // FLOAT type
    db.executeOne("CREATE DATABASE floattest");
    db.executeOne("USE floattest");
    db.executeOne("CREATE TABLE prices (id INTEGER PRIMARY KEY, price FLOAT)");
    db.executeOne("INSERT INTO prices VALUES (1, 3.14)");
    r = db.executeOne("SELECT * FROM prices WHERE price > 3.0");
    TEST("FLOAT type", r.success && r.rows.size() == 1);

    // BOOLEAN type
    db.executeOne("CREATE TABLE flags (id INTEGER PRIMARY KEY, active BOOLEAN)");
    db.executeOne("INSERT INTO flags VALUES (1, true), (2, false)");
    r = db.executeOne("SELECT * FROM flags WHERE active = true");
    TEST("BOOLEAN type", r.success && r.rows.size() == 1);

    // IS NULL
    db.executeOne("CREATE TABLE nulltest (id INTEGER PRIMARY KEY, val VARCHAR(10))");
    db.executeOne("INSERT INTO nulltest VALUES (1, NULL)");
    db.executeOne("INSERT INTO nulltest VALUES (2, 'hello')");
    r = db.executeOne("SELECT * FROM nulltest WHERE val IS NULL");
    TEST("IS NULL", r.success && r.rows.size() == 1);
    r = db.executeOne("SELECT * FROM nulltest WHERE val IS NOT NULL");
    TEST("IS NOT NULL", r.success && r.rows.size() == 1);

    // Unknown table
    r = db.executeOne("SELECT * FROM nonexistent");
    TEST("Unknown table error", !r.success);

    // Unknown column
    r = db.executeOne("SELECT zzz FROM flags");
    TEST("Unknown column error", !r.success);

    // Syntax error
    r = db.executeOne("SELEKT * FORM users");
    TEST("Syntax error handled", !r.success);

    cleanup();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
