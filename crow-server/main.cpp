#include "crow.h"
#include <curl/curl.h>
#include <sw/redis++/redis++.h>
#include <sstream>

// Функция для записи ответа curl в std::string
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

int main()

{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/api")([]()
                            {
        try {
            std::random_device rd;
            int random_value = rd();

            sw::redis::Redis* redis = nullptr;
            try {
                redis = new sw::redis::Redis("tcp://redis:6379");
            } catch (const std::exception& e) {
                return crow::response(500, std::string("Redis connect error: ") + e.what());
            }

            std::string key = "key_" + std::to_string(random_value);
            // Получение JSON с локального сервиса faker
            std::string json_result;
            CURL* curl = curl_easy_init();
            if(curl) {
                curl_easy_setopt(curl, CURLOPT_URL, "http://faker:3000/user");
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_result);
                CURLcode res = curl_easy_perform(curl);
                curl_easy_cleanup(curl);
                if(res != CURLE_OK) {
                    delete redis;
                    return crow::response(500, "Failed to fetch JSON: " + std::string(curl_easy_strerror(res)));
                }
            } else {
                delete redis;
                return crow::response(500, "Failed to init curl");
            }

            // ...existing code...
            try {
                redis->set(key, json_result);
            } catch (const std::exception& e) {
                delete redis;
                return crow::response(500, std::string("Redis SET error: ") + e.what());
            }

            std::string val_str;
            // ...existing code...
            try {
                auto val = redis->get(key);
                val_str = val ? *val : "";
            } catch (const std::exception& e) {
                delete redis;
                return crow::response(500, std::string("Redis GET error: ") + e.what());
            }

            delete redis;
            std::stringstream ss;
            ss << "{\"key\":\"" << key << "\",\"value\":\"" << val_str << "\"}";
            return crow::response(200, ss.str());
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        } });

    app.port(8080).multithreaded().run();
}
