
# Базовый образ Yandex.Tank (содержит Phantom и все необходимые зависимости)
FROM yandex/yandex-tank:latest

# Установить системные зависимости для Python-библиотек
RUN apt-get update && \
    apt-get install -y build-essential python3-dev libffi-dev gfortran libssl-dev

# Обновить pip и setuptools
RUN pip3 install --upgrade pip setuptools

# Установить Flask и boto3 (для S3, если нужно)
RUN pip3 install flask boto3

# Создать рабочую директорию
WORKDIR /app

# Копировать слушатель
COPY tank_listener.py /app/

# Открыть порт для Flask
EXPOSE 3001

# Точка входа
ENTRYPOINT ["python3", "/app/tank_listener.py"]

