#include "DBMS.h"
#include "httplib.h"
#include "json.hpp"
#include <iostream>
#include <string>

using json = nlohmann::json;

int main() {
    DBMS dbms;
    httplib::Server svr;

    // Функция для автоматического навешивания CORS заголовков
    auto set_cors_headers = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    };

    // Обработка Preflight (OPTIONS) запросов от браузера
    svr.Options(R"(/api/.*)", [&](const httplib::Request&, httplib::Response& res) {
        set_cors_headers(res);
        res.status = 200;
    });

    // Главный эндпоинт для выполнения SQL запросов из UI
    svr.Post("/api/execute", [&](const httplib::Request& req, httplib::Response& res) {
        set_cors_headers(res);

        try {
            auto input_json = json::parse(req.body);
            std::string sql = input_json.value("query", "");

            if (sql.empty()) {
                res.status = 400;
                res.set_content(json{{"status", "error"}, {"message", "Query is empty"}}.dump(), "application/json");
                return;
            }

            // Вызываем родной метод СУБД
            auto results = dbms.execute(sql);

            json json_results = json::array();
            for (auto& r : results) {
                json_results.push_back({
                    {"status", "success"},
                    {"output", r.toString()}
                });
            }

            res.set_content(json_results.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"status", "error"}, {"message", e.what()}}.dump(), "application/json");
        }
    });

    std::cout << ">> MyDB Server started at http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
