# Проект: Микросервисная нагрузочная платформа

## Описание

Этот проект представляет собой микросервисную платформу для генерации и тестирования нагрузки на HTTP API. Включает несколько сервисов: C++ сервер на Crow, Node.js сервер-генератор данных, Redis для кеширования, Portainer для управления контейнерами и инструменты для нагрузочного тестирования (Yandex Tank).

## Структура

- **crow-server/** — C++ HTTP API сервер на базе Crow, использует Redis и Curl.
- **faker-server/** — Node.js сервис, генерирующий фейковые пользовательские данные через Faker.js.
- **loadtest/** — Скрипты и конфиги для нагрузочного тестирования (Yandex Tank).
- **docker-compose.yml** — Описывает инфраструктуру и связи между сервисами.

## Быстрый старт

1. **Требования**: Docker, docker-compose.
2. **Запуск всех сервисов**:
   ```sh
   docker-compose up --build
   ```
3. **Доступные сервисы**:
   - Crow API: [http://localhost:8080/api](http://localhost:8080/api)
   - Faker API: [http://localhost:3000/user](http://localhost:3000/user)
   - Portainer: [http://localhost:9000](http://localhost:9000)
   - Redis: [localhost:6379](localhost:6379)

## Описание сервисов

### crow-server

- Язык: C++ (Crow, Boost, Redis++)
- Эндпоинт `/api`:
  - Получает случайные данные пользователя с faker-server.
  - Кеширует результат в Redis.
  - Логгирует действия через spdlog.

### faker-server

- Язык: Node.js (Express, @faker-js/faker, winston)
- Эндпоинт `/user`:
  - Генерирует случайного пользователя (id, имя, email, аватар, дата рождения, адрес).
  - Логгирует запросы.

### loadtest

- Скрипты и конфиги для Yandex Tank.
- Пример конфигурации: `load.yaml` (тестирует crow-server по адресу `/api`).

## Тестирование нагрузки

1. Перейдите в папку `loadtest/`.
2. Запустите Yandex Tank (через docker-compose профиль `loadtest`):
   ```sh
   docker-compose run --rm --profile loadtest yandex-tank
   ```
3. Логи и результаты появятся в папке `loadtest/logs/`.

## Управление контейнерами

- Для управления используйте Portainer: [http://localhost:9000](http://localhost:9000)

## Зависимости

- C++: Crow, Boost, Redis++, hiredis, curl, spdlog
- Node.js: express, @faker-js/faker, winston
- Docker images: redis, portainer/portainer-ce, yandex/yandex-tank

## Пример запроса

```sh
curl http://localhost:8080/api
```

## Авторы и лицензия

- Для учебных и демонстрационных целей.
