from flask import Flask, request, jsonify
import subprocess
import os


import signal

app = Flask(__name__)

# Глобально храним pid процесса tank (если запущен)
tank_pid = None
tank_proc = None


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
    global tank_pid, tank_proc
    proc = subprocess.Popen([
        'yandex-tank', '-c', config_path
    ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, preexec_fn=os.setsid)
    tank_pid = proc.pid
    tank_proc = proc


@app.route('/stop-tank', methods=['POST'])
def stop_tank():
    global tank_pid, tank_proc
    if not tank_pid:
        return jsonify({'message': 'Нет запущенного теста'}), 400
    try:
        # Сначала мягко SIGINT всей группе
        os.killpg(os.getpgid(tank_pid), signal.SIGINT)
        # Ждём завершения процесса (не блокируем Flask)
        if tank_proc:
            try:
                tank_proc.wait(timeout=10)
            except Exception:
                pass
        # На всякий случай добиваем всю группу SIGKILL
        try:
            os.killpg(os.getpgid(tank_pid), signal.SIGKILL)
        except Exception:
            pass
        msg = f'Тест (pid={tank_pid}) остановлен.'
        tank_pid = None
        tank_proc = None
        return jsonify({'message': msg})
    except Exception as e:
        return jsonify({'message': f'Ошибка остановки: {e}'}), 500


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


# Новый статус: анализ tank.log
@app.route('/test-status', methods=['GET'])
def test_status():
    import glob
    import os
    log_dir = '/app/logs'
    try:
        # Найти последнюю папку с логами
        subdirs = [
            os.path.join(
                log_dir,
                d) for d in os.listdir(log_dir) if os.path.isdir(
                os.path.join(
                    log_dir,
                    d))]
        if not subdirs:
            return jsonify({'running': False, 'reason': 'Нет логов'}), 200
        latest_dir = max(subdirs, key=os.path.getmtime)
        tank_log = os.path.join(latest_dir, 'tank.log')
        if not os.path.exists(tank_log):
            return jsonify({'running': False, 'reason': 'Нет tank.log'}), 200
        # Читаем файл с конца блоками по 20 строк, ищем ключевые строки
        running = None
        with open(tank_log, 'rb') as f:
            f.seek(0, os.SEEK_END)
            file_size = f.tell()
            block_size = 4096
            pos = file_size
            leftover = b''
            lines_found = 0
            while pos > 0 and lines_found < 100:  # максимум 100 строк назад
                read_size = min(block_size, pos)
                pos -= read_size
                f.seek(pos)
                block = f.read(read_size) + leftover
                lines = block.split(b'\n')
                if pos > 0:
                    leftover = lines[0]
                    lines = lines[1:]
                else:
                    leftover = b''
                # Идём с конца блока
                for l in reversed(lines):
                    line = l.decode('utf-8', errors='ignore').strip()
                    if not line:
                        continue
                    lines_found += 1
                    if (
                        'Phantom done its work with exit code' in line or
                        'Finishing test' in line
                    ):
                        running = False
                        break
                    if 'Waiting for test to finish' in line:
                        running = True
                        break
                if running is not None:
                    break
        if running is None:
            running = False
        return jsonify({'running': running})
    except Exception as e:
        return jsonify({'running': False, 'error': str(e)}), 200


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=3001)
