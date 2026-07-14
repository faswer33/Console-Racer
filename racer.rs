// racer.rs - Гоночная игра в консоли (Rust)
use crossterm::{
    event::{self, Event, KeyCode},
    execute,
    terminal::{self, Clear, ClearType},
};
use std::io::{stdout, Write};
use std::time::{Duration, Instant};
use std::collections::VecDeque;
use serde::{Serialize, Deserialize};
use std::fs;
use rand::Rng;

const WIDTH: usize = 11;
const HEIGHT: usize = 15;
const ROAD_LEFT: usize = 2;
const ROAD_RIGHT: usize = WIDTH - 3;
const CAR: char = '▲';
const OBSTACLE: char = '█';
const RECORD_FILE: &str = "racer_record.json";

#[derive(Serialize, Deserialize)]
struct RecordData {
    record: u32,
}

struct Racer {
    player_pos: usize,
    obstacles: VecDeque<(usize, usize)>,
    score: u32,
    speed: u32,
    game_over: bool,
    paused: bool,
    running: bool,
    record: u32,
    rng: rand::rngs::ThreadRng,
}

impl Racer {
    fn new() -> Self {
        Self {
            player_pos: WIDTH / 2,
            obstacles: VecDeque::new(),
            score: 0,
            speed: 1,
            game_over: false,
            paused: false,
            running: true,
            record: Self::load_record(),
            rng: rand::thread_rng(),
        }
    }

    fn load_record() -> u32 {
        if let Ok(data) = fs::read_to_string(RECORD_FILE) {
            if let Ok(rec) = serde_json::from_str::<RecordData>(&data) {
                return rec.record;
            }
        }
        0
    }

    fn save_record(record: u32) {
        let data = RecordData { record };
        let _ = fs::write(RECORD_FILE, serde_json::to_string(&data).unwrap());
    }

    fn spawn_obstacle(&mut self) {
        let x = self.rng.gen_range(ROAD_LEFT..=ROAD_RIGHT);
        self.obstacles.push_back((x, 0));
    }

    fn update(&mut self) {
        if self.game_over || self.paused {
            return;
        }

        // Сдвиг вниз
        for item in self.obstacles.iter_mut() {
            item.1 += 1;
        }
        // Удалить вышедшие
        self.obstacles.retain(|&(_, y)| y < HEIGHT);

        // Столкновение
        for &(x, y) in &self.obstacles {
            if y == HEIGHT - 1 && x == self.player_pos {
                self.game_over = true;
                if self.score > self.record {
                    self.record = self.score;
                    Self::save_record(self.record);
                }
                return;
            }
        }
        self.score += 1;
        self.speed = 1 + self.score / 10;

        // Спавн
        let prob = 0.3 + f64::min(0.4, self.score as f64 / 200.0);
        if self.rng.gen_bool(prob) {
            self.spawn_obstacle();
        }
    }

    fn draw(&self) {
        execute!(stdout(), Clear(ClearType::All)).unwrap();
        let mut out = stdout();
        writeln!(out, "{}", "═".repeat(WIDTH)).unwrap();
        writeln!(out, "  Счёт: {}   Рекорд: {}   Скорость: {}", self.score, self.record, self.speed).unwrap();
        writeln!(out, "{}", "═".repeat(WIDTH)).unwrap();

        for y in 0..HEIGHT {
            let mut line = String::with_capacity(WIDTH);
            for x in 0..WIDTH {
                if x == 0 || x == WIDTH - 1 {
                    line.push('│');
                } else {
                    let is_obs = self.obstacles.iter().any(|&(ox, oy)| ox == x && oy == y);
                    if is_obs {
                        line.push(OBSTACLE);
                    } else if y == HEIGHT - 1 && x == self.player_pos {
                        line.push(CAR);
                    } else {
                        line.push(' ');
                    }
                }
            }
            writeln!(out, "{}", line).unwrap();
        }
        writeln!(out, "{}", "═".repeat(WIDTH)).unwrap();
        let status = if self.paused { "ПАУЗА" } else { "ИГРА" };
        writeln!(out, "  {}  |  ← → движение  |  P - пауза  |  R - рестарт  |  Q - выход", status).unwrap();
        out.flush().unwrap();
    }

    fn reset(&mut self) {
        self.player_pos = WIDTH / 2;
        self.obstacles.clear();
        self.score = 0;
        self.speed = 1;
        self.game_over = false;
        self.paused = false;
    }

    fn run(&mut self) {
        // Настройка терминала для raw режима
        terminal::enable_raw_mode().unwrap();
        let mut last_update = Instant::now();

        while self.running {
            // Обработка событий клавиатуры
            if event::poll(Duration::from_millis(50)).unwrap() {
                if let Event::Key(key) = event::read().unwrap() {
                    match key.code {
                        KeyCode::Left | KeyCode::Char('a') | KeyCode::Char('A') => {
                            if !self.game_over && !self.paused && self.player_pos > ROAD_LEFT {
                                self.player_pos -= 1;
                            }
                        }
                        KeyCode::Right | KeyCode::Char('d') | KeyCode::Char('D') => {
                            if !self.game_over && !self.paused && self.player_pos < ROAD_RIGHT {
                                self.player_pos += 1;
                            }
                        }
                        KeyCode::Char('p') | KeyCode::Char('P') => {
                            self.paused = !self.paused;
                        }
                        KeyCode::Char('r') | KeyCode::Char('R') => {
                            self.reset();
                        }
                        KeyCode::Char('q') | KeyCode::Char('Q') => {
                            self.running = false;
                        }
                        _ => {}
                    }
                }
            }

            // Обновление по времени
            let now = Instant::now();
            if now - last_update >= Duration::from_secs_f64(1.0 / self.speed as f64) {
                self.update();
                self.draw();
                last_update = now;
            }
        }

        terminal::disable_raw_mode().unwrap();
    }
}

fn main() {
    let mut racer = Racer::new();
    racer.run();
    println!("Игра завершена.");
}
