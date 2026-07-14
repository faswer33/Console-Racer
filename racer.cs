// racer.cs - Гоночная игра в консоли (C#)
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading;
using System.Linq;

class Racer
{
    private const int WIDTH = 11;
    private const int HEIGHT = 15;
    private const int ROAD_LEFT = 2;
    private const int ROAD_RIGHT = WIDTH - 3;
    private const char CAR = '▲';
    private const char OBSTACLE = '█';
    private const string RECORD_FILE = "racer_record.json";

    private int playerPos;
    private List<(int x, int y)> obstacles;
    private int score;
    private int speed;
    private bool gameOver;
    private bool paused;
    private bool running;
    private int record;
    private Random rand;

    public Racer()
    {
        playerPos = WIDTH / 2;
        obstacles = new List<(int, int)>();
        score = 0;
        speed = 1;
        gameOver = false;
        paused = false;
        running = true;
        record = LoadRecord();
        rand = new Random();
    }

    private int LoadRecord()
    {
        if (!File.Exists(RECORD_FILE)) return 0;
        string json = File.ReadAllText(RECORD_FILE);
        var data = JsonSerializer.Deserialize<Dictionary<string, int>>(json);
        return data != null && data.ContainsKey("record") ? data["record"] : 0;
    }

    private void SaveRecord()
    {
        var data = new Dictionary<string, int> { { "record", record } };
        string json = JsonSerializer.Serialize(data);
        File.WriteAllText(RECORD_FILE, json);
    }

    private void SpawnObstacle()
    {
        int x = rand.Next(ROAD_LEFT, ROAD_RIGHT + 1);
        obstacles.Add((x, 0));
    }

    private void Update()
    {
        if (gameOver || paused) return;

        // Сдвиг вниз
        for (int i = obstacles.Count - 1; i >= 0; i--)
        {
            var obs = obstacles[i];
            obs.y++;
            obstacles[i] = obs;
            if (obs.y >= HEIGHT) obstacles.RemoveAt(i);
        }

        // Столкновение
        foreach (var obs in obstacles)
        {
            if (obs.y == HEIGHT - 1 && obs.x == playerPos)
            {
                gameOver = true;
                if (score > record)
                {
                    record = score;
                    SaveRecord();
                }
                return;
            }
        }

        score++;
        speed = 1 + score / 10;

        // Спавн
        double prob = 0.3 + Math.Min(0.4, score / 200.0);
        if (rand.NextDouble() < prob)
            SpawnObstacle();
    }

    private void Draw()
    {
        Console.Clear();
        Console.WriteLine(new string('═', WIDTH));
        Console.WriteLine($"  Счёт: {score}   Рекорд: {record}   Скорость: {speed}");
        Console.WriteLine(new string('═', WIDTH));

        for (int y = 0; y < HEIGHT; y++)
        {
            for (int x = 0; x < WIDTH; x++)
            {
                if (x == 0 || x == WIDTH - 1)
                    Console.Write('│');
                else
                {
                    bool isObs = obstacles.Any(o => o.x == x && o.y == y);
                    if (isObs)
                        Console.Write(OBSTACLE);
                    else if (y == HEIGHT - 1 && x == playerPos)
                        Console.Write(CAR);
                    else
                        Console.Write(' ');
                }
            }
            Console.WriteLine();
        }
        Console.WriteLine(new string('═', WIDTH));
        Console.WriteLine($"  {(paused ? "ПАУЗА" : "ИГРА")}  |  ← → движение  |  P - пауза  |  R - рестарт  |  Q - выход");
    }

    private void Reset()
    {
        playerPos = WIDTH / 2;
        obstacles.Clear();
        score = 0;
        speed = 1;
        gameOver = false;
        paused = false;
    }

    public void Run()
    {
        DateTime lastUpdate = DateTime.Now;
        while (running)
        {
            // Обработка ввода (неблокирующая)
            while (Console.KeyAvailable)
            {
                var key = Console.ReadKey(true);
                switch (key.Key)
                {
                    case ConsoleKey.LeftArrow:
                    case ConsoleKey.A:
                        if (!gameOver && !paused) playerPos = Math.Max(ROAD_LEFT, playerPos - 1);
                        break;
                    case ConsoleKey.RightArrow:
                    case ConsoleKey.D:
                        if (!gameOver && !paused) playerPos = Math.Min(ROAD_RIGHT, playerPos + 1);
                        break;
                    case ConsoleKey.P:
                        paused = !paused;
                        break;
                    case ConsoleKey.R:
                        Reset();
                        break;
                    case ConsoleKey.Q:
                        running = false;
                        break;
                }
            }

            // Обновление по таймеру
            var now = DateTime.Now;
            if ((now - lastUpdate).TotalSeconds >= 1.0 / speed)
            {
                Update();
                Draw();
                lastUpdate = now;
            }
            Thread.Sleep(20);
        }
    }

    static void Main()
    {
        var game = new Racer();
        game.Run();
        Console.WriteLine("Игра завершена.");
    }
}
