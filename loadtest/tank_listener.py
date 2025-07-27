from flask import Flask, request, jsonify
import subprocess
import os

app = Flask(__name__)


@app.route('/run-tank', methods=['POST'])
def run_tank():
    data = request.json
    rps = int(data.get('rps', 100))
    duration = data.get('duration', '60s')
    import re
    # Очистка lock-файла, если остался от предыдущего теста
    lock_path = '/var/lock'
    try:
        for fname in os.listdir(lock_path):
            if fname.startswith('lunapark_') and fname.endswith('.lock'):
                os.remove(os.path.join(lock_path, fname))
    except Exception:
        pass

    # Сбор зомби-процессов (если есть)
    try:
        while True:
            # -1: любой дочерний процесс, os.WNOHANG: не блокировать
            pid, _ = os.waitpid(-1, os.WNOHANG)
            if pid == 0:
                break
    except ChildProcessError:
        pass

    if not re.match(r'^[0-9]+[smh]$', str(duration)):
        return jsonify(
            {'message': 'Некорректный формат времени. Пример: 60s, 5m, 2h'}), 400
    # Генерируем корректный конфиг: только json_report
    config = f"""phantom:
  address: crow-server:8080
  uris:
    - /api
  load_profile:
    load_type: rps
    schedule: const({rps}, {duration})
  writelog: all
console:
  enabled: true
telegraf:
  enabled: false
json_report:
  enabled: true
"""
    config_path = '/var/loadtest/load.yaml'
    with open(config_path, 'w') as f:
        f.write(config)
    # Запуск теста
    proc = subprocess.Popen([
        'yandex-tank', '-c', config_path
    ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return jsonify({'message': 'Тест запущен', 'pid': proc.pid})


@app.route('/last-report', methods=['GET'])
def last_report():
    import glob
    import json
    logs_dir = '/app/logs'
    import os
    try:
        # Найти все папки с датой
        subdirs = [
            os.path.join(
                logs_dir,
                d) for d in os.listdir(logs_dir) if os.path.isdir(
                os.path.join(
                    logs_dir,
                    d))]
        if not subdirs:
            return jsonify({'message': 'Нет отчётов'}), 404
        latest_dir = max(subdirs, key=os.path.getmtime)
        test_data_log = os.path.join(latest_dir, 'test_data.log')
        if not os.path.exists(test_data_log):
            return jsonify({'message': 'test_data.log не найден'}), 404
        # Прочитать последнюю строку
        with open(test_data_log, 'rb') as f:
            try:
                f.seek(-4096, os.SEEK_END)
            except OSError:
                f.seek(0)
            lines = f.readlines()
            if not lines:
                return jsonify({'message': 'test_data.log пуст'}), 404
            last_line = lines[-1].decode('utf-8').strip()
        import json
        try:
            data = json.loads(last_line)
        except Exception:
            data = {'raw': last_line}
        return jsonify(data)
    except Exception as e:
        return jsonify({'message': f'Ошибка чтения отчёта: {e}'}), 500


@app.route('/')
def index():
    return 'Yandex.Tank listener is running.'


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=3001)
