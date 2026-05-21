#include "DBMS.h"
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

void printBanner() {
    std::cout << R"(
  _____                 _____ ____  ____  __  __  _____
 |  ___|__  ____ _   __|  _  |  _ \|  _ \|  \/  |/ ____|
 | |_ / _ \/ __ \ \ / /| |_| | |_) | | | | \  / | (___
 |  _| (_) \__ \ V V / |  _  |  __/| |_| | |\/| |\___ \
 |_|  \___/|___/ \_/\_/ |_| |_|_|   |____/|_|  |_|_____/

 Своя СУБД v1.0  |  Кейс-чемпионат
 Авторы: Шубенина Д.В., Рыжкин А.А.
 Введите SQL-запрос или 'exit' для выхода.
 Многострочный ввод: завершите запрос символом ';'
)";
}

void printHelp() {
    std::cout << "\n--- Справка ---\n"
              << "DDL команды:\n"
              << "  CREATE DATABASE <name>;\n"
              << "  DROP DATABASE <name>;\n"
              << "  USE <name>;\n"
              << "  SHOW DATABASES;\n"
              << "  CREATE TABLE <name> (col TYPE [constraints], ...);\n"
              << "  DROP TABLE <name>;\n"
              << "  SHOW TABLES;\n"
              << "\nDML команды:\n"
              << "  SELECT * FROM <table> [WHERE cond];\n"
              << "  SELECT col1, col2 FROM <table> [WHERE cond];\n"
              << "  INSERT INTO <table> [(cols)] VALUES (vals) [, (vals)...];\n"
              << "  UPDATE <table> SET col=val [WHERE cond];\n"
              << "  DELETE FROM <table> [WHERE cond];\n"
              << "\nТипы данных: INTEGER, FLOAT, VARCHAR(n), BOOLEAN\n"
              << "Ограничения: PRIMARY KEY, NOT NULL, UNIQUE, FOREIGN KEY REFERENCES\n"
              << "Операторы WHERE: =, !=, <, >, <=, >=, AND, OR, IS NULL, IS NOT NULL\n"
              << "\nСлужебные команды:\n"
              << "  \\help   -- показать эту справку\n"
              << "  \\source <file> -- выполнить SQL из файла\n"
              << "  exit    -- выход\n\n";
}

int main(int argc, char* argv[]) {
    DBMS dbms("./data");

    // Non-interactive mode: read from file argument
    if (argc == 2) {
        std::ifstream f(argv[1]);
        if (!f.is_open()) {
            std::cerr << "Cannot open file: " << argv[1] << "\n";
            return 1;
        }
        std::ostringstream ss; ss << f.rdbuf();
        auto results = dbms.execute(ss.str());
        for (auto& r : results) {
            if (!r.columns.empty() || r.success)
                std::cout << r.toString() << "\n";
            else
                std::cout << r.toString() << "\n";
        }
        return 0;
    }

    printBanner();

    std::string line, buffer;
    bool multiline = false;

    auto printPrompt = [&]() {
        if (dbms.currentDb().empty())
            std::cout << "mydb> ";
        else
            std::cout << dbms.currentDb() << "> ";
        std::cout.flush();
    };

    while (true) {
        printPrompt();
        if (!std::getline(std::cin, line)) break; // EOF

        // Trim
        while (!line.empty() && isspace((unsigned char)line.back())) line.pop_back();

        if (line == "exit" || line == "quit" || line == "\\q") break;
        if (line == "\\help" || line == "help") { printHelp(); continue; }

        // Source file command
        if (line.size() > 8 && line.substr(0, 7) == "\\source") {
            std::string fname = line.substr(8);
            while (!fname.empty() && fname.front() == ' ') fname.erase(fname.begin());
            std::ifstream f(fname);
            if (!f.is_open()) { std::cout << "Cannot open file: " << fname << "\n"; continue; }
            std::ostringstream ss; ss << f.rdbuf();
            auto results = dbms.execute(ss.str());
            for (auto& r : results) std::cout << r.toString() << "\n";
            continue;
        }

        buffer += line;

        // Check if statement is complete (ends with ;)
        bool complete = false;
        for (int i = (int)buffer.size()-1; i >= 0; i--) {
            if (buffer[i] == ';') { complete = true; break; }
            if (!isspace((unsigned char)buffer[i])) break;
        }

        if (complete) {
            auto results = dbms.execute(buffer);
            for (auto& r : results) {
                std::cout << r.toString() << "\n";
            }
            buffer.clear();
        } else {
            buffer += "\n";
            // Show continuation prompt on next iteration
        }
    }

    std::cout << "\nДо свидания!\n";
    return 0;
}
