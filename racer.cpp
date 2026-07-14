// racer.cpp - Гоночная игра в консоли (C++17)
#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>
#include <string>
#include <cstdlib>
#include <algorithm>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #define CLEAR() system("cls")
#else
    #include <ncurses.h>
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define CLEAR() system("clear")
#endif

const int WIDTH = 11;
const int HEIGHT = 15;
const int ROAD_LEFT = 2;
const int ROAD_RIGHT = WIDTH - 3;
const char CAR = '▲';
const char OBSTACLE = '█';
const std::string RECORD_FILE = "racer_record.json";

class Racer {
public:
    Racer() : playerPos(WIDTH/2), score(0), speed(1), gameOver(false), paused(false), running(true) {
        record = loadRecord();
        rng = std::mt19937(std::random_device{}());
    }

    ~Racer() {
#ifdef _WIN32
        // ничего
#else
        endwin();
#endif
    }

    void run() {
#ifdef _WIN32
        // Для Windows используем _kbhit и getch в цикле
        while (running) {
            update();
            draw();
            handleInputWindows();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / speed));
        }
#else
        // Для Unix используем ncurses
        initscr();
        raw();
        noecho();
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);
        curs_set(0);

        while (running) {
            update();
            drawNcurses();
            handleInputNcurses();
            napms(1000 / speed);
        }
        endwin();
#endif
    }

private:
    int playerPos;
    std::vector<std::pair<int,int>> obstacles; // (x, y)
    int score;
    int speed;
    bool gameOver;
    bool paused;
    bool running;
    int record;
    std::mt19937 rng;

    int loadRecord() {
        std::ifstream in(RECORD_FILE);
        if (!in) return 0;
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        // Простой парсинг
        size_t pos = content.find("\"record\":");
        if (pos != std::string::npos) {
            pos += 9;
            size_t end = content.find(",", pos);
            if (end == std::string::npos) end = content.find("}", pos);
            return std::stoi(content.substr(pos, end - pos));
        }
        return 0;
    }

    void saveRecord() {
        std::ofstream out(RECORD_FILE);
        out << "{\"record\":" << record << "}";
    }

    void spawnObstacle() {
        std::uniform_int_distribution<int> dist(ROAD_LEFT, ROAD_RIGHT);
        int x = dist(rng);
        obstacles.push_back({x, 0});
    }

    void update() {
        if (gameOver || paused) return;

        // Сдвиг
        for (auto& obs : obstacles) obs.second++;
        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
            [this](const auto& obs) { return obs.second >= HEIGHT; }), obstacles.end());

        // Столкновение
        for (const auto& obs : obstacles) {
            if (obs.second == HEIGHT - 1 && obs.first == playerPos) {
                gameOver = true;
                if (score > record) {
                    record = score;
                    saveRecord();
                }
                return;
            }
        }
        score++;
        speed = 1 + score / 10;

        // Спавн
        double prob = 0.3 + std::min(0.4, score / 200.0);
        if ((double)rand() / RAND_MAX < prob) {
            spawnObstacle();
        }
    }

    void draw() {
        CLEAR();
        std::cout << std::string(WIDTH, '═') << std::endl;
        std::cout << "  Счёт: " << score << "   Рекорд: " << record << "   Скорость: " << speed << std::endl;
        std::cout << std::string(WIDTH, '═') << std::endl;

        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                if (x == 0 || x == WIDTH - 1) {
                    std::cout << '│';
                } else {
                    bool isObs = false;
                    for (const auto& obs : obstacles) {
                        if (obs.first == x && obs.second == y) {
                            isObs = true;
                            break;
                        }
                    }
                    if (isObs) std::cout << OBSTACLE;
                    else if (y == HEIGHT - 1 && x == playerPos) std::cout << CAR;
                    else std::cout << ' ';
                }
            }
            std::cout << std::endl;
        }
        std::cout << std::string(WIDTH, '═') << std::endl;
        std::cout << "  " << (paused ? "ПАУЗА" : "ИГРА") << "  |  ← → движение  |  P - пауза  |  R - рестарт  |  Q - выход" << std::endl;
    }

#ifdef _WIN32
    void handleInputWindows() {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 224) { // стрелки
                ch = _getch();
                switch (ch) {
                    case 75: // left
                        if (!gameOver && !paused) playerPos = std::max(ROAD_LEFT, playerPos - 1);
                        break;
                    case 77: // right
                        if (!gameOver && !paused) playerPos = std::min(ROAD_RIGHT, playerPos + 1);
                        break;
                }
            } else {
                switch (tolower(ch)) {
                    case 'a':
                        if (!gameOver && !paused) playerPos = std::max(ROAD_LEFT, playerPos - 1);
                        break;
                    case 'd':
                        if (!gameOver && !paused) playerPos = std::min(ROAD_RIGHT, playerPos + 1);
                        break;
                    case 'p': paused = !paused; break;
                    case 'r': reset(); break;
                    case 'q': running = false; break;
                }
            }
        }
    }
#else
    void drawNcurses() {
        clear();
        printw("%s\n", std::string(WIDTH, '═').c_str());
        printw("  Счёт: %d   Рекорд: %d   Скорость: %d\n", score, record, speed);
        printw("%s\n", std::string(WIDTH, '═').c_str());

        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                if (x == 0 || x == WIDTH - 1) {
                    addch('│');
                } else {
                    bool isObs = false;
                    for (const auto& obs : obstacles) {
                        if (obs.first == x && obs.second == y) {
                            isObs = true;
                            break;
                        }
                    }
                    if (isObs) addch(OBSTACLE);
                    else if (y == HEIGHT - 1 && x == playerPos) addch(CAR);
                    else addch(' ');
                }
            }
            addch('\n');
        }
        printw("%s\n", std::string(WIDTH, '═').c_str());
        printw("  %s  |  ← → движение  |  P - пауза  |  R - рестарт  |  Q - выход\n",
               paused ? "ПАУЗА" : "ИГРА");
        refresh();
    }

    void handleInputNcurses() {
        int ch = getch();
        if (ch == ERR) return;
        switch (ch) {
            case KEY_LEFT:
            case 'a': case 'A':
                if (!gameOver && !paused) playerPos = std::max(ROAD_LEFT, playerPos - 1);
                break;
            case KEY_RIGHT:
            case 'd': case 'D':
                if (!gameOver && !paused) playerPos = std::min(ROAD_RIGHT, playerPos + 1);
                break;
            case 'p': case 'P': paused = !paused; break;
            case 'r': case 'R': reset(); break;
            case 'q': case 'Q': running = false; break;
        }
    }
#endif

    void reset() {
        playerPos = WIDTH / 2;
        obstacles.clear();
        score = 0;
        speed = 1;
        gameOver = false;
        paused = false;
    }
};

int main() {
    Racer game;
    game.run();
    return 0;
}
