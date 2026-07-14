// racer.js - Гоночная игра в консоли (JavaScript/Node.js)
const fs = require('fs');
const readline = require('readline');
const keypress = require('keypress');

// Настройка keypress
keypress(process.stdin);

const WIDTH = 11;
const HEIGHT = 15;
const ROAD_LEFT = 2;
const ROAD_RIGHT = WIDTH - 3;
const CAR = '▲';
const OBSTACLE = '█';
const RECORD_FILE = 'racer_record.json';

class RacerGame {
    constructor() {
        this.width = WIDTH;
        this.height = HEIGHT;
        this.playerPos = Math.floor(WIDTH / 2);
        this.obstacles = []; // {x, y}
        this.score = 0;
        this.speed = 1;
        this.gameOver = false;
        this.paused = false;
        this.record = this.loadRecord();
        this.running = true;
        this.timer = null;
    }

    loadRecord() {
        try {
            const data = fs.readFileSync(RECORD_FILE, 'utf8');
            return JSON.parse(data).record || 0;
        } catch {
            return 0;
        }
    }

    saveRecord() {
        fs.writeFileSync(RECORD_FILE, JSON.stringify({ record: this.record }));
    }

    spawnObstacle() {
        const x = Math.floor(Math.random() * (ROAD_RIGHT - ROAD_LEFT + 1)) + ROAD_LEFT;
        this.obstacles.push({ x, y: 0 });
    }

    update() {
        if (this.gameOver || this.paused) return;

        // Сдвиг препятствий
        for (let obs of this.obstacles) {
            obs.y++;
        }
        this.obstacles = this.obstacles.filter(obs => obs.y < this.height);

        // Проверка столкновения
        for (let obs of this.obstacles) {
            if (obs.y === this.height - 1 && obs.x === this.playerPos) {
                this.gameOver = true;
                if (this.score > this.record) {
                    this.record = this.score;
                    this.saveRecord();
                }
                return;
            }
        }
        this.score++;
        this.speed = 1 + Math.floor(this.score / 10);

        // Спавн
        if (Math.random() < 0.3 + Math.min(0.4, this.score / 200)) {
            this.spawnObstacle();
        }
    }

    draw() {
        console.clear();
        console.log('═'.repeat(this.width));
        console.log(`  Счёт: ${this.score}   Рекорд: ${this.record}   Скорость: ${this.speed}`);
        console.log('═'.repeat(this.width));

        for (let y = 0; y < this.height; y++) {
            let line = '';
            for (let x = 0; x < this.width; x++) {
                if (x === 0 || x === this.width - 1) {
                    line += '│';
                } else {
                    const obs = this.obstacles.find(o => o.x === x && o.y === y);
                    if (obs) {
                        line += OBSTACLE;
                    } else if (y === this.height - 1 && x === this.playerPos) {
                        line += CAR;
                    } else {
                        line += ' ';
                    }
                }
            }
            console.log(line);
        }
        console.log('═'.repeat(this.width));
        const status = this.paused ? 'ПАУЗА' : 'ИГРА';
        console.log(`  ${status}  |  ← → движение  |  P - пауза  |  R - рестарт  |  Q - выход`);
    }

    reset() {
        this.playerPos = Math.floor(this.width / 2);
        this.obstacles = [];
        this.score = 0;
        this.speed = 1;
        this.gameOver = false;
        this.paused = false;
    }

    handleKey(ch, key) {
        if (!key) return;
        if (key.name === 'left' || key.name === 'a') {
            if (!this.gameOver && !this.paused) {
                this.playerPos = Math.max(ROAD_LEFT, this.playerPos - 1);
            }
        } else if (key.name === 'right' || key.name === 'd') {
            if (!this.gameOver && !this.paused) {
                this.playerPos = Math.min(ROAD_RIGHT, this.playerPos + 1);
            }
        } else if (key.name === 'p') {
            this.paused = !this.paused;
        } else if (key.name === 'r') {
            this.reset();
        } else if (key.name === 'q') {
            this.running = false;
            process.stdin.pause();
            process.exit(0);
        }
    }

    run() {
        // Настройка stdin
        process.stdin.setRawMode(true);
        process.stdin.resume();
        process.stdin.on('keypress', (ch, key) => this.handleKey(ch, key));

        // Основной цикл
        const gameLoop = () => {
            if (!this.running) return;
            this.update();
            this.draw();
            this.timer = setTimeout(gameLoop, 1000 / this.speed);
        };
        gameLoop();
    }
}

// Запуск
const game = new RacerGame();
game.run();
