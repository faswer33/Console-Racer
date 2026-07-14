
---

## 💻 Код на 7 языках

### 1. Python – `racer.py`

```python
#!/usr/bin/env python3
# racer.py - Гоночная игра в консоли (Python)

import os
import sys
import time
import random
import json
import threading
from collections import deque

try:
    import keyboard
except ImportError:
    print("Установите keyboard: pip install keyboard")
    sys.exit(1)

# Константы
WIDTH = 11          # нечётное число для симметрии
HEIGHT = 15
ROAD_LEFT = 2
ROAD_RIGHT = WIDTH - 3
CAR = '▲'
OBSTACLE = '█'
EMPTY = ' '
RECORD_FILE = 'racer_record.json'

class RacerGame:
    def __init__(self):
        self.width = WIDTH
        self.height = HEIGHT
        self.player_pos = self.width // 2
        self.obstacles = []  # список (x, y)
        self.score = 0
        self.speed = 1       # шагов в секунду
        self.game_over = False
        self.paused = False
        self.record = self.load_record()
        self.tick = 0
        self.lock = threading.Lock()
        self.running = True

    def load_record(self):
        try:
            with open(RECORD_FILE, 'r') as f:
                data = json.load(f)
                return data.get('record', 0)
        except:
            return 0

    def save_record(self):
        with open(RECORD_FILE, 'w') as f:
            json.dump({'record': self.record}, f)

    def spawn_obstacle(self):
        # Случайная позиция на дороге (без учёта краёв)
        x = random.randint(ROAD_LEFT, ROAD_RIGHT)
        self.obstacles.append([x, 0])

    def update(self):
        if self.game_over or self.paused:
            return

        # Сдвиг препятствий вниз
        for obs in self.obstacles:
            obs[1] += 1
        # Удалить вышедшие за экран
        self.obstacles = [obs for obs in self.obstacles if obs[1] < self.height]
        # Проверка столкновения
        for obs in self.obstacles:
            if obs[1] == self.height - 1 and obs[0] == self.player_pos:
                self.game_over = True
                if self.score > self.record:
                    self.record = self.score
                    self.save_record()
                return
        # Счёт
        self.score += 1
        # Увеличение скорости каждые 10 очков
        self.speed = 1 + self.score // 10

        # Спавн новых препятствий с вероятностью
        if random.random() < 0.3 + min(0.4, self.score / 200):
            self.spawn_obstacle()

    def draw(self):
        os.system('cls' if os.name == 'nt' else 'clear')
        # Верхняя граница
        print('═' * self.width)
        print(f'  Счёт: {self.score}   Рекорд: {self.record}   Скорость: {self.speed}')
        print('═' * self.width)
        # Игровое поле
        for y in range(self.height):
            line = []
            for x in range(self.width):
                if x == 0 or x == self.width - 1:
                    line.append('│')
                else:
                    # Проверяем препятствия
                    obs = None
                    for o in self.obstacles:
                        if o[1] == y and o[0] == x:
                            obs = o
                            break
                    if obs:
                        line.append(OBSTACLE)
                    elif y == self.height - 1 and x == self.player_pos:
                        line.append(CAR)
                    else:
                        line.append(EMPTY)
            print(''.join(line))
        print('═' * self.width)
        status = "ПАУЗА" if self.paused else "ИГРА"
        print(f'  {status}  |  ← → движение  |  P - пауза  |  R - рестарт  |  Q - выход')

    def handle_input(self):
        # Неблокирующее чтение клавиш с помощью keyboard
        while self.running:
            try:
                if keyboard.is_pressed('left') or keyboard.is_pressed('a'):
                    with self.lock:
                        if not self.game_over and not self.paused:
                            self.player_pos = max(ROAD_LEFT, self.player_pos - 1)
                elif keyboard.is_pressed('right') or keyboard.is_pressed('d'):
                    with self.lock:
                        if not self.game_over and not self.paused:
                            self.player_pos = min(ROAD_RIGHT, self.player_pos + 1)
                elif keyboard.is_pressed('p'):
                    with self.lock:
                        self.paused = not self.paused
                    time.sleep(0.2)  # debounce
                elif keyboard.is_pressed('r'):
                    with self.lock:
                        self.reset()
                    time.sleep(0.2)
                elif keyboard.is_pressed('q'):
                    self.running = False
                    break
            except:
                pass
            time.sleep(0.05)

    def reset(self):
        self.player_pos = self.width // 2
        self.obstacles = []
        self.score = 0
        self.speed = 1
        self.game_over = False
        self.paused = False
        self.tick = 0

    def run(self):
        # Запуск потока для ввода
        input_thread = threading.Thread(target=self.handle_input, daemon=True)
        input_thread.start()

        # Основной игровой цикл
        while self.running:
            with self.lock:
                self.update()
                self.draw()
            time.sleep(1.0 / self.speed)
            self.tick += 1

        print("Игра завершена.")

if __name__ == "__main__":
    game = RacerGame()
    try:
        game.run()
    except KeyboardInterrupt:
        print("\nВыход...")
