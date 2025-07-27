#include <curl/curl.h>
#include <sw/redis++/redis++.h>
#include <sstream>
#include "crow.h"
#include <cmark.h>

#include <spdlog/spdlog.h>
// Логируем только в stdout/stderr (для Loki Docker logging driver)

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

int main()
{
    spdlog::set_level(spdlog::level::info); // Уровень логирования INFO
    spdlog::set_pattern("(%Y-%m-%d %H:%M:%S) [%-8l] %v");

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Info); // Crow будет выводить только INFO и выше

    CROW_ROUTE(app, "/api")([]()
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
                curl_easy_setopt(curl, CURLOPT_URL, "http://faker:3000/user");
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

    CROW_ROUTE(app, "/docs")([]()
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

    app.port(8080).multithreaded().run();
}
