# Микросервисная нагрузочная платформа

## Описание


Микросервисная платформа для генерации и тестирования нагрузки на HTTP API. Включает:
- C++ сервер на Crow (crow-server)
- Node.js сервер-генератор данных (faker-server)
- Redis для кеширования
- Portainer для управления контейнерами
- Инструменты нагрузочного тестирования (Yandex Tank)

## Структура

- **crow-server/** — C++ HTTP API сервер на базе Crow, использует Redis и Curl.
- **faker-server/** — Node.js сервис, генерирующий фейковые пользовательские данные через Faker.js.
- **loadtest/** — Скрипты и конфиги для нагрузочного тестирования (Yandex Tank).
- **docker-compose.yml** — Описывает инфраструктуру и связи между сервисами.


## Быстрый старт

1. **Требования**: Docker, docker-compose
2. **Сборка и запуск**:
   ```sh
   docker-compose up --build
   ```
3. **Сервисы по умолчанию**:
   - Crow API: http://localhost:8080/api
   - Faker API: http://localhost:3000/user
   - Portainer: http://localhost:9000
   - Redis: localhost:6379
   - Grafana: http://localhost:3002/
   - Prometheus: http://localhost:9090/

## Описание сервисов


### crow-server

- Язык: C++ (Crow, Boost, Redis++)
- Эндпоинты:
  - `/api` — получает случайные данные пользователя с faker-server, кеширует результат в Redis, логгирует действия через spdlog
  - `/docs` — возвращает HTML-документацию, сгенерированную из README.md


### faker-server

- Язык: Node.js (Express, @faker-js/faker, winston)
- Эндпоинт `/user` — генерирует случайного пользователя (id, имя, email, аватар, дата рождения, адрес), логгирует запросы


### loadtest

- Скрипты и конфиги для Yandex Tank
- Пример: `load.yaml` (тестирует crow-server по адресу `/api`)


## Тестирование нагрузки

1. Перейдите в папку `loadtest/`
2. Запустите Yandex Tank:
   ```sh
   docker-compose run --rm --profile loadtest yandex-tank
   ```
3. Логи и результаты появятся в папке `loadtest/logs/`


## Управление контейнерами

- Для управления используйте Portainer: http://localhost:9000


## Зависимости

- C++: Crow, Boost, Redis++, hiredis, curl, spdlog
- Node.js: express, @faker-js/faker, winston
- Docker images: redis, portainer/portainer-ce, yandex/yandex-tank


## Мониторинг и метрики

- **Prometheus** (http://localhost:9090/) собирает метрики с crow-server (порт 8081) и других сервисов.
- **Grafana** (http://localhost:3002/) визуализирует метрики (RPS, latency percentiles, ошибки и др.).
- Основные метрики crow-server:
  - `http_request_duration_seconds` — histogram (latency, автоматически создаёт *_count, *_sum, *_bucket)
  - `http_requests_total` — counter (общее количество HTTP-запросов)

### Пример запроса к метрикам

```sh
curl http://localhost:8081/metrics
```

### Пример запроса RPS в Prometheus/Grafana

```
sum(rate(http_request_duration_seconds_count{job="crow-server-metrics"}[2s]))
```

## Пример запроса

```sh
curl http://localhost:8080/api
```


## Авторы и лицензия

- Для учебных и демонстрационных целей
