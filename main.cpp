#include "crow.h"

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/api")
    ([]()
     { return "Hello world"; });

    app.port(8080).multithreaded().run();
}
