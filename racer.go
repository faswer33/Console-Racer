// racer.go - Гоночная игра в консоли (Go)
package main

import (
	"encoding/json"
	"fmt"
	"math/rand"
	"os"
	"os/exec"
	"runtime"
	"time"

	"github.com/eiannone/keyboard"
)

const (
	WIDTH      = 11
	HEIGHT     = 15
	ROAD_LEFT  = 2
	ROAD_RIGHT = WIDTH - 3
	CAR        = '▲'
	OBSTACLE   = '█'
	RECORD_FILE = "racer_record.json"
)

type Racer struct {
	playerPos int
	obstacles [][2]int // [x, y]
	score     int
	speed     int
	gameOver  bool
	paused    bool
	running   bool
	record    int
}

func NewRacer() *Racer {
	return &Racer{
		playerPos: WIDTH / 2,
		obstacles: make([][2]int, 0),
		score:     0,
		speed:     1,
		gameOver:  false,
		paused:    false,
		running:   true,
		record:    loadRecord(),
	}
}

func loadRecord() int {
	file, err := os.Open(RECORD_FILE)
	if err != nil {
		return 0
	}
	defer file.Close()
	var data map[string]int
	decoder := json.NewDecoder(file)
	if err := decoder.Decode(&data); err != nil {
		return 0
	}
	if val, ok := data["record"]; ok {
		return val
	}
	return 0
}

func saveRecord(record int) {
	data := map[string]int{"record": record}
	file, _ := os.Create(RECORD_FILE)
	defer file.Close()
	encoder := json.NewEncoder(file)
	encoder.Encode(data)
}

func (r *Racer) spawnObstacle() {
	x := ROAD_LEFT + rand.Intn(ROAD_RIGHT-ROAD_LEFT+1)
	r.obstacles = append(r.obstacles, [2]int{x, 0})
}

func (r *Racer) update() {
	if r.gameOver || r.paused {
		return
	}

	// Сдвиг вниз
	newObs := make([][2]int, 0, len(r.obstacles))
	for _, obs := range r.obstacles {
		obs[1]++
		if obs[1] < HEIGHT {
			newObs = append(newObs, obs)
		}
	}
	r.obstacles = newObs

	// Столкновение
	for _, obs := range r.obstacles {
		if obs[1] == HEIGHT-1 && obs[0] == r.playerPos {
			r.gameOver = true
			if r.score > r.record {
				r.record = r.score
				saveRecord(r.record)
			}
			return
		}
	}
	r.score++
	r.speed = 1 + r.score/10

	// Спавн
	prob := 0.3 + min(0.4, float64(r.score)/200.0)
	if rand.Float64() < prob {
		r.spawnObstacle()
	}
}

func (r *Racer) draw() {
	clearScreen()
	fmt.Println(stringRepeat("═", WIDTH))
	fmt.Printf("  Счёт: %d   Рекорд: %d   Скорость: %d\n", r.score, r.record, r.speed)
	fmt.Println(stringRepeat("═", WIDTH))

	for y := 0; y < HEIGHT; y++ {
		line := make([]rune, WIDTH)
		for x := 0; x < WIDTH; x++ {
			if x == 0 || x == WIDTH-1 {
				line[x] = '│'
			} else {
				isObs := false
				for _, obs := range r.obstacles {
					if obs[0] == x && obs[1] == y {
						isObs = true
						break
					}
				}
				if isObs {
					line[x] = OBSTACLE
				} else if y == HEIGHT-1 && x == r.playerPos {
					line[x] = CAR
				} else {
					line[x] = ' '
				}
			}
		}
		fmt.Println(string(line))
	}
	fmt.Println(stringRepeat("═", WIDTH))
	status := "ИГРА"
	if r.paused {
		status = "ПАУЗА"
	}
	fmt.Printf("  %s  |  ← → движение  |  P - пауза  |  R - рестарт  |  Q - выход\n", status)
}

func (r *Racer) reset() {
	r.playerPos = WIDTH / 2
	r.obstacles = nil
	r.score = 0
	r.speed = 1
	r.gameOver = false
	r.paused = false
}

func (r *Racer) handleInput() {
	for r.running {
		char, key, err := keyboard.GetKey()
		if err != nil {
			continue
		}
		switch key {
		case keyboard.KeyArrowLeft:
			if !r.gameOver && !r.paused {
				if r.playerPos > ROAD_LEFT {
					r.playerPos--
				}
			}
		case keyboard.KeyArrowRight:
			if !r.gameOver && !r.paused {
				if r.playerPos < ROAD_RIGHT {
					r.playerPos++
				}
			}
		case keyboard.KeyPgUp, keyboard.KeyPgDn: // ignore
		default:
			switch char {
			case 'a', 'A':
				if !r.gameOver && !r.paused {
					if r.playerPos > ROAD_LEFT {
						r.playerPos--
					}
				}
			case 'd', 'D':
				if !r.gameOver && !r.paused {
					if r.playerPos < ROAD_RIGHT {
						r.playerPos++
					}
				}
			case 'p', 'P':
				r.paused = !r.paused
			case 'r', 'R':
				r.reset()
			case 'q', 'Q':
				r.running = false
				return
			}
		}
	}
}

func clearScreen() {
	cmd := exec.Command("clear")
	if runtime.GOOS == "windows" {
		cmd = exec.Command("cmd", "/c", "cls")
	}
	cmd.Stdout = os.Stdout
	cmd.Run()
}

func stringRepeat(s string, n int) string {
	res := ""
	for i := 0; i < n; i++ {
		res += s
	}
	return res
}

func min(a, b float64) float64 {
	if a < b {
		return a
	}
	return b
}

func (r *Racer) run() {
	go r.handleInput()

	ticker := time.NewTicker(time.Second / 10) // 10 fps
	defer ticker.Stop()

	lastUpdate := time.Now()
	for r.running {
		select {
		case <-ticker.C:
			now := time.Now()
			if now.Sub(lastUpdate) >= time.Second/time.Duration(r.speed) {
				r.update()
				r.draw()
				lastUpdate = now
			}
		}
	}
}

func main() {
	rand.Seed(time.Now().UnixNano())
	racer := NewRacer()
	racer.run()
	fmt.Println("Игра завершена.")
}
