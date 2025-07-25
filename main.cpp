#include "crow.h"
#include <sw/redis++/redis++.h>
#include <sstream>

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/api")([]()
                            {
        try {
            // Эмуляция вычислений
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            std::random_device rd;
            int random_value = rd();

            sw::redis::Redis redis("tcp://redis:6379");
            std::string key = "key_" + std::to_string(random_value);
            redis.set(key, std::to_string(random_value));

            auto val = redis.get(key);
            std::stringstream ss;
            ss << "{\"key\":\"" << key << "\",\"value\":\"" << (val ? *val : "") << "\"}";
            return crow::response(200, ss.str());
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        } });

    app.port(8080).multithreaded().run();
}
