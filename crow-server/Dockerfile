FROM alpine:latest

# Установить необходимые пакеты
RUN apk add --no-cache build-base cmake git boost-dev hiredis-dev curl-dev fmt-dev

# Копировать исходники
WORKDIR /app
COPY . /app

# Собрать проект
RUN mkdir build && cd build && cmake .. && make -j$(nproc)

EXPOSE 8080

CMD ["/app/build/main"]
