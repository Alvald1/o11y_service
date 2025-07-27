# Install nlohmann_json (header-only, via CMake FetchContent)
# No system package needed, CMake will fetch it automatically
FROM alpine:latest

# Установить необходимые пакеты
RUN apk add --no-cache build-base cmake git boost-dev hiredis-dev curl-dev fmt-dev cmark-dev

# Создать кеш для FetchContent
ENV FETCHCONTENT_BASE_DIR=/fc_cache
RUN mkdir -p /fc_cache
VOLUME /fc_cache


# Копировать только CMakeLists.txt для зависимостей
WORKDIR /app
COPY crow-server/CMakeLists.txt /app/

# Сборка и установка зависимостей в отдельную папку (deps_install)
RUN mkdir -p build && cd build && cmake .. && cmake --build . --target deps_only -j$(nproc)

# Копировать остальные исходники
COPY crow-server/main.cpp /app

# Собрать проект, используя уже установленные зависимости
RUN cd build && cmake .. && cmake --build . --target main -j$(nproc)

COPY README.md /app
COPY crow-server/ /app

EXPOSE 8080

CMD ["/app/build/main"]
