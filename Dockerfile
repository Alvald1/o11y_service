FROM ubuntu:24.04

# Установить необходимые пакеты
RUN apt-get update && \
    apt-get install -y build-essential cmake git libboost-all-dev && \
    rm -rf /var/lib/apt/lists/*

# Копировать исходники
WORKDIR /app
COPY . /app

# Собрать проект
RUN mkdir build && cd build && cmake .. && make

# Открыть порт
EXPOSE 8080

# Запустить сервер
CMD ["/app/build/main"]
