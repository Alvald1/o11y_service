
#include <curl/curl.h>
#include <regex>

#include <sw/redis++/redis++.h>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "crow.h"
#include <cmark.h>

#include <spdlog/spdlog.h>
// Логируем только в stdout/stderr (для Loki Docker logging driver)

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string get_config_value(const nlohmann::json &config, const std::string &key)
{
    if (config.contains(key) && config[key].is_string())
    {
        return config[key].get<std::string>();
    }
    throw std::runtime_error("Config key not found: " + key);
}

int main()
{
    spdlog::set_level(spdlog::level::info); // Уровень логирования INFO
    spdlog::set_pattern("(%Y-%m-%d %H:%M:%S) [%-8l] %v");

    // Чтение конфигурации
    nlohmann::json config_json;
    {
        std::ifstream config_file("config.json");
        if (!config_file.is_open())
        {
            spdlog::error("Could not open config.json");
            throw std::runtime_error("Could not open config.json");
        }
        config_file >> config_json;
    }

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Info); // Crow будет выводить только INFO и выше

    CROW_ROUTE(app, "/api")([&]()
                            {
        try
        {
            std::random_device rd;
            int random_value = rd();

            sw::redis::Redis *redis = nullptr;
            try
            {
                spdlog::info("Connected to Redis");
                redis = new sw::redis::Redis("tcp://redis:6379");
            }
            catch (const std::exception &e)
            {
                spdlog::error("Redis connect error: {}", e.what());
                return crow::response(500, std::string("Redis connect error: ") + e.what());
            }

            std::string key = "key_" + std::to_string(random_value);
            std::string json_result;

            CURL *curl = curl_easy_init();
            if (curl)
            {
                std::string faker_url = get_config_value(config_json, "faker_url");
                curl_easy_setopt(curl, CURLOPT_URL, faker_url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_result);
                CURLcode res = curl_easy_perform(curl);
                curl_easy_cleanup(curl);
                if (res != CURLE_OK)
                {
                    spdlog::error("Failed to fetch JSON: {}", curl_easy_strerror(res));
                    delete redis;
                    return crow::response(500, "Failed to fetch JSON: " + std::string(curl_easy_strerror(res)));
                }
            }
            else
            {
                spdlog::error("Failed to init curl");
                delete redis;
                return crow::response(500, "Failed to init curl");
            }

            spdlog::info("Redis SET key: {}", key);
            try
            {
                redis->set(key, json_result);
            }
            catch (const std::exception &e)
            {
                spdlog::error("Redis SET error: {}", e.what());
                delete redis;
                return crow::response(500, std::string("Redis SET error: ") + e.what());
            }

            std::string val_str;
            spdlog::info("Redis GET key: {}", key);
            try
            {
                auto val = redis->get(key);
                val_str = val ? *val : "";
            }
            catch (const std::exception &e)
            {
                spdlog::error("Redis GET error: {}", e.what());
                delete redis;
                return crow::response(500, std::string("Redis GET error: ") + e.what());
            }

            delete redis;
            std::stringstream ss;
            ss << "{\"key\":\"" << key << "\",\"value\":\"" << val_str << "\"}";
            return crow::response(200, ss.str());
        }
        catch (const std::exception &e)
        {
            spdlog::error("Exception: {}", e.what());
            return crow::response(500, e.what());
        } });

    // Прокси-эндпоинт для остановки теста
    CROW_ROUTE(app, "/load/stop").methods("POST"_method)([&]()
                                                         {
        try {
            std::string tank_listener_url = get_config_value(config_json, "tank_listener_url");
            // Заменить путь на /stop-tank
            std::string url = tank_listener_url;
            size_t pos = url.find("/run-tank");
            if (pos != std::string::npos) {
                url.replace(pos, std::string("/run-tank").size(), "/stop-tank");
            } else {
                url = "http://yandex-tank:3001/stop-tank";
            }
            std::string response_string;
            CURL *curl = curl_easy_init();
            if (!curl) {
                return crow::response(500, R"({\"message\":\"Ошибка инициализации curl\"})");
            }
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{}");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                return crow::response(500, std::string("{\"message\":\"Ошибка запроса к tank-listener: ") + curl_easy_strerror(res) + "\"}");
            }
            return crow::response(200, response_string);
        } catch (const std::exception& e) {
            return crow::response(500, std::string("{\"message\":\"") + e.what() + "\"}");
        } });

    CROW_ROUTE(app, "/docs")([&]()
                             {
        std::ifstream file("README.md");
        if (!file.is_open()) {
            return crow::response(500, "Could not open documentation file.");
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        std::string markdown = buffer.str();

        char *html_c = cmark_markdown_to_html(markdown.c_str(), markdown.size(), CMARK_OPT_DEFAULT);
        std::string html = "<html><head><meta charset='utf-8'><title>Документация</title>"
            "<style>body{font-family:sans-serif;max-width:800px;margin:auto;padding:2em;}pre{background:#f4f4f4;padding:1em;}code{background:#eee;padding:2px 4px;}</style>"
            "</head><body>";
        html += html_c;
        html += "</body></html>";
        free(html_c);
        return crow::response(200, html); });

    CROW_ROUTE(app, "/load").methods("GET"_method)([&]()
                                                   {
        std::ifstream file("load.html");
        if (!file.is_open()) {
            return crow::response(500, "Could not open load.html");
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return crow::response(200, buffer.str()); });

    CROW_ROUTE(app, "/load").methods("POST"_method)([&](const crow::request &req)
                                                    {
        try {
            auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, R"({\"message\":\"Некорректный JSON\"})");
            }
            int rps = body["rps"].i();
            std::string duration_str;
            if (body["duration"].t() == crow::json::type::String) {
                duration_str = body["duration"].s();
                // Валидация через regex: ^[0-9]+[smh]$
                std::regex re("^[0-9]+[smh]$");
                if (!std::regex_match(duration_str, re)) {
                    return crow::response(400, R"({\"message\":\"Некорректный формат duration. Пример: 60s, 5m, 2h\"})");
                }
            } else {
                return crow::response(400, R"({\"message\":\"Некорректные параметры\"})");
            }
            if (rps < 1 || duration_str.empty()) {
                return crow::response(400, R"({\"message\":\"Некорректные параметры\"})");
            }

            // Отправляем запрос на tank-listener
            crow::json::wvalue tank_req;
            tank_req["rps"] = rps;
            tank_req["duration"] = duration_str;

            CURL *curl = curl_easy_init();
            if (!curl) {
                return crow::response(500, R"({\"message\":\"Ошибка инициализации curl\"})");
            }
            std::string response_string;
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            std::string tank_listener_url = get_config_value(config_json, "tank_listener_url");
            curl_easy_setopt(curl, CURLOPT_URL, tank_listener_url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            std::string json_body = tank_req.dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                return crow::response(500, std::string("{\"message\":\"Ошибка запроса к tank-listener: ") + curl_easy_strerror(res) + "\"}");
            }
            return crow::response(200, response_string);
        } catch (const std::exception& e) {
            return crow::response(500, std::string("{\"message\":\"") + e.what() + "\"}");
        } });

    // Прокси-эндпоинт для получения статистики теста с yandex-tank
    CROW_ROUTE(app, "/load/report")([&]()
                                    {
        try {
            std::string tank_listener_url = get_config_value(config_json, "tank_listener_url");
            // Заменить путь на /last-report
            std::string url = tank_listener_url;
            size_t pos = url.find("/run-tank");
            if (pos != std::string::npos) {
                url.replace(pos, std::string("/run-tank").size(), "/last-report");
            } else {
                // fallback: просто заменить на /last-report
                url = "http://yandex-tank:3001/last-report";
            }
            std::string report_json;
            CURL *curl = curl_easy_init();
            if (!curl) {
                return crow::response(500, R"({\"message\":\"Ошибка инициализации curl\"})");
            }
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &report_json);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                return crow::response(500, std::string("{\"message\":\"Ошибка запроса к tank-listener: ") + curl_easy_strerror(res) + "\"}");
            }
            // Отдать как есть (json)
            return crow::response(200, report_json);
        } catch (const std::exception& e) {
            return crow::response(500, std::string("{\"message\":\"") + e.what() + "\"}");
        } });
    CROW_ROUTE(app, "/load/status")([&]()
                                    {
        try {
            std::string tank_listener_url = get_config_value(config_json, "tank_listener_url");
            // Заменить путь на /test-status
            std::string url = tank_listener_url;
            size_t pos = url.find("/run-tank");
            if (pos != std::string::npos) {
                url.replace(pos, std::string("/run-tank").size(), "/test-status");
            } else {
                url = "http://yandex-tank:3001/test-status";
            }
            std::string response_string;
            CURL *curl = curl_easy_init();
            if (!curl) {
                return crow::response(500, R"({\"message\":\"Ошибка инициализации curl\"})");
            }
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                return crow::response(500, std::string("{\"message\":\"Ошибка запроса к tank-listener: ") + curl_easy_strerror(res) + "\"}");
            }
            return crow::response(200, response_string);
        } catch (const std::exception& e) {
            return crow::response(500, std::string("{\"message\":\"") + e.what() + "\"}");
        } });
    app.port(8080).multithreaded().run();
}
