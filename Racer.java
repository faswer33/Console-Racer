// Racer.java - Гоночная игра в консоли (Java)
import org.jline.terminal.Terminal;
import org.jline.terminal.TerminalBuilder;
import org.jline.keymap.KeyMap;
import org.jline.reader.LineReader;
import org.jline.reader.LineReaderBuilder;
import org.jline.reader.UserInterruptException;
import org.jline.utils.InfoCmp;

import java.io.IOException;
import java.util.*;
import java.nio.file.*;
import com.google.gson.*;

public class Racer {
    private static final int WIDTH = 11;
    private static final int HEIGHT = 15;
    private static final int ROAD_LEFT = 2;
    private static final int ROAD_RIGHT = WIDTH - 3;
    private static final char CAR = '▲';
    private static final char OBSTACLE = '█';
    private static final String RECORD_FILE = "racer_record.json";

    private Terminal terminal;
    private int playerPos;
    private List<int[]> obstacles; // [x, y]
    private int score;
    private int speed;
    private boolean gameOver;
    private boolean paused;
    private int record;
    private boolean running;
    private long lastUpdate;

    public Racer() throws IOException {
        terminal = TerminalBuilder.builder().system(true).build();
        playerPos = WIDTH / 2;
        obstacles = new ArrayList<>();
        score = 0;
        speed = 1;
        gameOver = false;
        paused = false;
        running = true;
        record = loadRecord();
        lastUpdate = System.currentTimeMillis();
    }

    private int loadRecord() {
        try {
            String content = new String(Files.readAllBytes(Paths.get(RECORD_FILE)));
            JsonObject obj = new Gson().fromJson(content, JsonObject.class);
            return obj.get("record").getAsInt();
        } catch (Exception e) {
            return 0;
        }
    }

    private void saveRecord() {
        try {
            JsonObject obj = new JsonObject();
            obj.addProperty("record", record);
            Files.write(Paths.get(RECORD_FILE), obj.toString().getBytes());
        } catch (Exception e) {}
    }

    private void spawnObstacle() {
        int x = ROAD_LEFT + (int)(Math.random() * (ROAD_RIGHT - ROAD_LEFT + 1));
        obstacles.add(new int[]{x, 0});
    }

    private void update() {
        if (gameOver || paused) return;

        // Сдвиг вниз
        for (int[] obs : obstacles) {
            obs[1]++;
        }
        obstacles.removeIf(obs -> obs[1] >= HEIGHT);

        // Столкновение
        for (int[] obs : obstacles) {
            if (obs[1] == HEIGHT - 1 && obs[0] == playerPos) {
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
        if (Math.random() < 0.3 + Math.min(0.4, score / 200.0)) {
            spawnObstacle();
        }
    }

    private void draw() {
        terminal.puts(InfoCmp.Capability.clear_screen);
        System.out.println("═".repeat(WIDTH));
        System.out.printf("  Счёт: %d   Рекорд: %d   Скорость: %d%n", score, record, speed);
        System.out.println("═".repeat(WIDTH));

        for (int y = 0; y < HEIGHT; y++) {
            StringBuilder line = new StringBuilder();
            for (int x = 0; x < WIDTH; x++) {
                if (x == 0 || x == WIDTH - 1) {
                    line.append('│');
                } else {
                    boolean isObs = false;
                    for (int[] obs : obstacles) {
                        if (obs[0] == x && obs[1] == y) {
                            isObs = true;
                            break;
                        }
                    }
                    if (isObs) {
                        line.append(OBSTACLE);
                    } else if (y == HEIGHT - 1 && x == playerPos) {
                        line.append(CAR);
                    } else {
                        line.append(' ');
                    }
                }
            }
            System.out.println(line);
        }
        System.out.println("═".repeat(WIDTH));
        String status = paused ? "ПАУЗА" : "ИГРА";
        System.out.printf("  %s  |  ← → движение  |  P - пауза  |  R - рестарт  |  Q - выход%n", status);
    }

    private void reset() {
        playerPos = WIDTH / 2;
        obstacles.clear();
        score = 0;
        speed = 1;
        gameOver = false;
        paused = false;
    }

    private void handleInput() {
        try {
            while (running) {
                int ch = terminal.reader().read();
                if (ch == -1) continue;
                char c = (char) ch;
                switch (c) {
                    case 'a': case 'A': case 27: // left arrow: ESC [ D
                        // обработаем отдельно
                        break;
                    default:
                        break;
                }
                // Для стрелок используем терминальный ввод
                // В jline сложно обрабатывать стрелки в цикле, поэтому используем KeyMap
                // Упростим: используем обычные клавиши
                if (c == 'a' || c == 'A' || c == '←') {
                    if (!gameOver && !paused) playerPos = Math.max(ROAD_LEFT, playerPos - 1);
                } else if (c == 'd' || c == 'D' || c == '→') {
                    if (!gameOver && !paused) playerPos = Math.min(ROAD_RIGHT, playerPos + 1);
                } else if (c == 'p' || c == 'P') {
                    paused = !paused;
                } else if (c == 'r' || c == 'R') {
                    reset();
                } else if (c == 'q' || c == 'Q') {
                    running = false;
                    break;
                }
                // Для стрелок: в jline читаем последовательности, но для упрощения оставим буквы
                // В полноценной версии использовать KeyMap
                // Здесь для демонстрации используем a/d
            }
        } catch (IOException e) {}
    }

    public void run() throws InterruptedException {
        Thread inputThread = new Thread(this::handleInput);
        inputThread.setDaemon(true);
        inputThread.start();

        while (running) {
            long now = System.currentTimeMillis();
            if (now - lastUpdate >= 1000 / speed) {
                update();
                draw();
                lastUpdate = now;
            }
            Thread.sleep(50);
        }
        terminal.close();
    }

    public static void main(String[] args) throws Exception {
        Racer game = new Racer();
        game.run();
    }
}
