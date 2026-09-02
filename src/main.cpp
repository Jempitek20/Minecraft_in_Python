#include <M5Cardputer.h>
#include <Preferences.h>
#include <math.h>

static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 135;
static constexpr int POP = 24;
static constexpr int INPUTS = 4;
static constexpr int HIDDEN = 6;
static constexpr float BIRD_X = 45.0f;
static constexpr float PIPE_W = 18.0f;
static constexpr float GRAVITY = 0.22f;
static constexpr float FLAP = -3.8f;
static constexpr float SPEED = 1.8f;

struct Brain {
  float w1[INPUTS][HIDDEN];
  float b1[HIDDEN];
  float w2[HIDDEN];
  float b2;
  float fitness;
};

Brain population[POP];
Brain champion{};
Preferences prefs;

float birdY = 67.0f;
float birdV = 0.0f;
float pipeX = SCREEN_W;
float gapY = 67.0f;
int score = 0;
int generation = 1;
int bestScore = 0;

bool running = false;
bool training = true;

float frand(float lo, float hi) {
  return lo + (float)random(100000) / 100000.0f * (hi - lo);
}

float sigmoid(float x) {
  return 1.0f / (1.0f + expf(-x));
}

void randomBrain(Brain &b) {
  for (int i = 0; i < INPUTS; ++i)
    for (int j = 0; j < HIDDEN; ++j)
      b.w1[i][j] = frand(-2.0f, 2.0f);

  for (int j = 0; j < HIDDEN; ++j) {
    b.b1[j] = frand(-2.0f, 2.0f);
    b.w2[j] = frand(-2.0f, 2.0f);
  }
  b.b2 = frand(-2.0f, 2.0f);
  b.fitness = 0.0f;
}

float think(const Brain &b) {
  float in[INPUTS] = {
    birdY / SCREEN_H,
    birdV / 5.0f,
    (pipeX - BIRD_X) / SCREEN_W,
    (gapY - birdY) / SCREEN_H
  };

  float hidden[HIDDEN];
  for (int j = 0; j < HIDDEN; ++j) {
    float s = b.b1[j];
    for (int i = 0; i < INPUTS; ++i)
      s += in[i] * b.w1[i][j];
    hidden[j] = tanhf(s);
  }

  float out = b.b2;
  for (int j = 0; j < HIDDEN; ++j)
    out += hidden[j] * b.w2[j];
  return sigmoid(out);
}

void mutate(Brain &b, float amount) {
  for (int i = 0; i < INPUTS; ++i)
    for (int j = 0; j < HIDDEN; ++j)
      if (frand(0, 1) < 0.18f) b.w1[i][j] += frand(-amount, amount);

  for (int j = 0; j < HIDDEN; ++j) {
    if (frand(0, 1) < 0.18f) b.b1[j] += frand(-amount, amount);
    if (frand(0, 1) < 0.18f) b.w2[j] += frand(-amount, amount);
  }
  if (frand(0, 1) < 0.18f) b.b2 += frand(-amount, amount);
}

void newGame() {
  birdY = 67.0f;
  birdV = 0.0f;
  pipeX = SCREEN_W;
  gapY = frand(38.0f, 96.0f);
  score = 0;
}

bool crashed() {
  if (birdY < 5.0f || birdY > SCREEN_H - 5.0f) return true;
  if (pipeX + PIPE_W >= BIRD_X - 5.0f && pipeX <= BIRD_X + 5.0f) {
    if (birdY < gapY - 24.0f || birdY > gapY + 24.0f) return true;
  }
  return false;
}

void drawGame(const Brain &b, int gen, int fit) {
  auto &d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  d.fillRect((int)pipeX, 0, (int)PIPE_W, max(0, (int)(gapY - 24)), TFT_GREEN);
  int bottomY = (int)(gapY + 24);
  if (bottomY < SCREEN_H)
    d.fillRect((int)pipeX, bottomY, (int)PIPE_W, SCREEN_H - bottomY, TFT_GREEN);
  d.fillCircle((int)BIRD_X, (int)birdY, 5, TFT_YELLOW);

  d.setTextColor(TFT_WHITE);
  d.setTextSize(1);
  d.setCursor(3, 3);
  d.printf("GEN %d  SCORE %d", gen, score);
  d.setCursor(3, 14);
  d.printf("BEST %d  FIT %d", bestScore, fit);
  d.setCursor(3, 25);
  d.print(training ? "AI TRAINING" : "AI PLAY");
}

void saveChampion() {
  prefs.begin("neurofly", false);
  prefs.putBytes("brain", &champion, sizeof(champion));
  prefs.putInt("best", bestScore);
  prefs.putInt("gen", generation);
  prefs.end();
}

bool loadChampion() {
  prefs.begin("neurofly", true);
  size_t n = prefs.getBytes("brain", &champion, sizeof(champion));
  bestScore = prefs.getInt("best", 0);
  generation = prefs.getInt("gen", 1);
  prefs.end();
  return n == sizeof(champion);
}

int evaluate(Brain &b, bool draw) {
  newGame();
  unsigned long started = millis();
  int frames = 0;

  while (millis() - started < 12000UL && frames < 2500) {
    if (think(b) > 0.5f) birdV = FLAP;
    birdV += GRAVITY;
    birdY += birdV;
    pipeX -= SPEED;

    if (pipeX < -PIPE_W) {
      pipeX = SCREEN_W;
      gapY = frand(38.0f, 96.0f);
      ++score;
      if (score > bestScore) bestScore = score;
    }

    if (crashed()) break;

    if (draw) {
      drawGame(b, generation, (int)b.fitness);
      delay(8);
    }
    ++frames;
  }

  b.fitness = score * 1000.0f + frames;
  return score;
}

void makeNextGeneration() {
  int elite = 0;
  for (int i = 1; i < POP; ++i)
    if (population[i].fitness > population[elite].fitness) elite = i;

  champion = population[elite];
  bestScore = max(bestScore, (int)(champion.fitness / 1000.0f));
  saveChampion();

  for (int i = 0; i < POP; ++i) {
    if (i == 0) {
      population[i] = champion;
    } else {
      population[i] = champion;
      mutate(population[i], i < 6 ? 0.20f : 0.45f);
    }
  }
  ++generation;
}

void trainStep() {
  for (int i = 0; i < POP; ++i) {
    if (!running) return;
    evaluate(population[i], false);
    if (i % 4 == 0) {
      drawGame(population[i], generation, (int)population[i].fitness);
      M5Cardputer.update();
    }
  }
  makeNextGeneration();
}

void startTraining() {
  running = true;
  training = true;
  for (int i = 0; i < POP; ++i) randomBrain(population[i]);

  while (running) {
    trainStep();
    if (generation % 3 == 0) {
      drawGame(champion, generation, (int)champion.fitness);
    }
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      auto keys = M5Cardputer.Keyboard.keysState();
      for (char c : keys.word) {
        if (c == 'q' || c == 'Q') {
          running = false;
          break;
        }
      }
    }
  }
}

void playChampion() {
  Brain b = champion;
  running = true;
  training = false;
  newGame();

  while (running) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      auto keys = M5Cardputer.Keyboard.keysState();
      for (char c : keys.word) {
        if (c == 'q' || c == 'Q') running = false;
      }
    }

    if (think(b) > 0.5f) birdV = FLAP;
    birdV += GRAVITY;
    birdY += birdV;
    pipeX -= SPEED;

    if (pipeX < -PIPE_W) {
      pipeX = SCREEN_W;
      gapY = frand(38.0f, 96.0f);
      ++score;
      if (score > bestScore) bestScore = score;
    }

    if (crashed()) newGame();
    drawGame(b, generation, (int)b.fitness);
    delay(8);
  }
}

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
  M5Cardputer.Display.setRotation(1);
  randomSeed((unsigned long)esp_random());

  if (!loadChampion()) {
    randomBrain(champion);
    champion.fitness = 0;
  }

  M5Cardputer.Display.fillScreen(TFT_BLACK);
  M5Cardputer.Display.setTextColor(TFT_WHITE);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(5, 8);
  M5Cardputer.Display.println("NEURO FLAPPY");
  M5Cardputer.Display.setCursor(5, 25);
  M5Cardputer.Display.println("T = train AI");
  M5Cardputer.Display.setCursor(5, 38);
  M5Cardputer.Display.println("P = play AI");
  M5Cardputer.Display.setCursor(5, 51);
  M5Cardputer.Display.println("Q = stop");
}

void loop() {
  M5Cardputer.update();
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    delay(10);
    return;
  }

  auto keys = M5Cardputer.Keyboard.keysState();
  for (char c : keys.word) {
    if (c == 't' || c == 'T') startTraining();
    if (c == 'p' || c == 'P') playChampion();
  }
}
