/*~~~~~~~~~~~~~~~~~~~~~~ Directivas ~~~~~~~~~~~~~~~~~~~~~~*/
#define BAUD_RATE 115200

#include "DualCore.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Arduino.h"
#include "Audio.h"
#include "C:\Users\bruno\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.0.4\libraries\SD\src\SD.h"
#include "FS.h"
#include <ArduinoJson.h>

// Joystick Connections
#define HORZ_PIN 34
#define VERT_PIN 35
#define SEL_PIN  17

// microSD Card Reader connections
#define SD_CS          5
#define SPI_MOSI      23 
#define SPI_MISO      19
#define SPI_SCK       18

// I2S Connections
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

// Character IDs
#define PLAYER_RIGHT 0
#define PLAYER_LEFT 1
#define PLAYER_RUNNING_RIGHT 2
#define PLAYER_RUNNING_LEFT 3
#define OBSTACLE_RIGHT 4
#define OBSTACLE_LEFT 5
#define COIN 7

DualCoreESP32 DCESP32;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Audio audio;
const char* SCORES_FILE = "/scores.json";
const int MAX_SCORES = 4;

// Banderas
bool isIntro = true;
bool isMenu = false;
bool isGameRunning = false;
bool isGameOver = false;

// Game values
int scores[4] = {0}; // Array para almacenar los puntajes (máximo 4)
int menuSelectedOption = 0;
bool isPaused = false;
int selectedOption = 0; // 0 para la opción izquierda, 1 para la derecha

enum Direction {
  LEFT, // moverse a la izquierda
  RIGHT // moverse a la derecha
};

int xPlayerPos = 3;
int yPlayerPos = 1;
bool isJumping = false;
Direction playerDirection = RIGHT;

int xObstacleLowerPos = 15;
int xObstacleUpperPos = 22; // Aparece con desfase
Direction directionLower = LEFT;
Direction directionUpper = LEFT;

int score = 0;
uint8_t scoreDigits;
int xCoinPos;
int yCoinPos;

uint8_t caracter1[] = {
  B01100, B01100, B00000, B01100, B01100, B01100, B01100, B01110,
};

uint8_t caracter2[] = {
  B00110, B00110, B00000, B00110, B00110, B00110, B00110, B01110
};

uint8_t caracter3[] = {
  B01100, B01100, B00000, B01110, B11100, B01100, B11010, B10011,
};

uint8_t caracter4[] = {
  B00110, B00110, B00000, B01110, B00111, B00110, B01011, B11001
};

uint8_t caracter5[] = {
  B01110,B11111,B11010,B11101,B01110,B01000,B00110,B00000
};

uint8_t caracter6[] = {
  B01110,B11111,B01011,B10111,B01110,B00010,B01100,B00000
};

uint8_t caracter7[] = {
  B00000,B10101,B01110,B11011,B01110,B10101,B00000,B00000
};

// Tiempos para la multitarea
unsigned long previousTimeObstacle = 0;
unsigned long previousTimePlayer = 0;
unsigned long previousTimeJump = 0;

const unsigned long obstacleInterval = 300;  // Intervalo de tiempo para las barreras
const unsigned long playerMoveInterval = 100;  // Intervalo de tiempo para mover el jugador
const unsigned long jumpInterval = 200;  // Intervalo de tiempo para el salto
unsigned long currentTime;

/*~~~~~~~~~~~~~~~~~~~~~~~ Game ~~~~~~~~~~~~~~~~~~~~~~~*/
void Loop1(void *pvParameters) {
  showIntro();
  isGameRunning = true;

  for (;;) {
    currentTime = millis();

    if (isGameOver) {
      handleGameOver(selectedOption);
    } else if (isPaused) {
      handlePauseMenu(selectedOption, isPaused);
    } else if (isMenu) {
      handleMenu(selectedOption);
    } else if (isGameRunning) {
      // Verificar si se presiona el botón de pausa
      if (digitalRead(SEL_PIN) == LOW) {
        isPaused = true;
        selectedOption = 0;
        showPauseMenu(selectedOption);
        delay(200); // Debounce
        continue;
      }

      // Lógica del juego existente
      updatePlayer();
      updateObstacles();
      saveScore(score);
    }
  }
}

void handleGameOver(int &selectedOption) {
  static bool optionsShown = false;

  if (!optionsShown) {
    showGameOverOptions(selectedOption);
    optionsShown = true;
  }

  // Manejar la selección de opciones
  if (analogRead(HORZ_PIN) == 4095) {
    selectedOption = 1;
    showGameOverOptions(selectedOption);
  } else if (analogRead(HORZ_PIN) == 0) {
    selectedOption = 0;
    showGameOverOptions(selectedOption);
  }

  if (digitalRead(SEL_PIN) == LOW) {
    if (selectedOption == 0) {
      // Reiniciar juego
      resetGame();
      isGameOver = false;
      isGameRunning = true;
    } else {
      goToMenu();
    }
    optionsShown = false;
    delay(200); // Debounce
  }
}

void handlePauseMenu(int &selectedOption, bool &isPaused) {
  // Manejar la selección de opciones
  if (analogRead(HORZ_PIN) == 4095) {
    selectedOption = 1;
    showPauseMenu(selectedOption);
  } else if (analogRead(HORZ_PIN) == 0) {
    selectedOption = 0;
    showPauseMenu(selectedOption);
  }

  if (digitalRead(SEL_PIN) == LOW) {
    if (selectedOption == 0) {
      // Reanudar juego
      isPaused = false;
      lcd.clear();
      // Redibujar el estado del juego
      redrawGameState();
    } else {
      goToMenu();
    }
    delay(200); // Debounce
  }
}

void showGameOverOptions(int selectedOption) {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("GAME OVER");
  lcd.setCursor(0, 1);
  lcd.print(selectedOption == 0 ? ">" : " ");
  lcd.print("Reiniciar");
  lcd.setCursor(11, 1);
  lcd.print(selectedOption == 1 ? ">" : " ");
  lcd.print("Menu");
}

void showPauseMenu(int selectedOption) {
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("PAUSA");
  lcd.setCursor(0, 1);
  lcd.print(selectedOption == 0 ? ">" : " ");
  lcd.print("Reanudar");
  lcd.setCursor(10, 1);
  lcd.print(selectedOption == 1 ? ">" : " ");
  lcd.print("Menu");
}

void goToMenu() {
  isGameRunning = false;
  isGameOver = false;
  isPaused = false;
  isMenu = true;
  selectedOption = 0;
  showMenu(selectedOption);
}

void showMenu(int selectedOption) {
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("MENU");
  lcd.setCursor(0, 1);
  lcd.print(selectedOption == 0 ? ">" : " ");
  lcd.print("Iniciar");
  lcd.setCursor(9, 1);
  lcd.print(selectedOption == 1 ? ">" : " ");
  lcd.print("Scores");
}

void handleMenu(int &selectedOption) {
  // Manejar la selección de opciones
  if (analogRead(HORZ_PIN) == 4095) {
    selectedOption = 1;
    showMenu(selectedOption);
  } else if (analogRead(HORZ_PIN) == 0) {
    selectedOption = 0;
    showMenu(selectedOption);
  }

  if (digitalRead(SEL_PIN) == LOW) {
    if (selectedOption == 0) {
      // Iniciar juego
      isMenu = false;
      isGameRunning = true;
      resetGame();
    } else {
      showScores();
    }
    delay(200); // Debounce
  }
}

void showScores() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scores:");
  for (int i = 0; i < 4; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(i + 1);        // Mostrar la posición
    lcd.print(". Puntaje: ");
    lcd.print(scores[i]);     // Mostrar el puntaje correspondiente
    delay(2000);              // Pausa de 2 segundos para cada puntaje
  }
  lcd.clear();
  showMenu(selectedOption);  // Regresar al menú después de mostrar los puntajes
}

void resetGame() {
  xPlayerPos = 3;
  yPlayerPos = 1;
  isJumping = false;
  xObstacleLowerPos = 15;
  xObstacleUpperPos = 22;
  directionLower = LEFT;
  directionUpper = LEFT;
  score = 0;
  spawnCoin();
  lcd.clear();
}

void redrawGameState() {
  // Redibujar el jugador
  lcd.setCursor(xPlayerPos, yPlayerPos);
  lcd.write(playerDirection == RIGHT ? PLAYER_RIGHT : PLAYER_LEFT);
  
  // Redibujar los obstáculos
  lcd.setCursor(xObstacleLowerPos, 1);
  lcd.write(directionLower == LEFT ? OBSTACLE_LEFT : OBSTACLE_RIGHT);
  lcd.setCursor(xObstacleUpperPos, 0);
  lcd.write(directionUpper == LEFT ? OBSTACLE_LEFT : OBSTACLE_RIGHT);
  
  // Redibujar la moneda
  drawCoin();
  
  // Mostrar el puntaje
  scoreDigits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;
  lcd.setCursor(16 - scoreDigits, 0);
  lcd.print(score);
}

void updatePlayer() {
  if (currentTime - previousTimePlayer > playerMoveInterval) {
    previousTimePlayer = currentTime;

    // Leer el estado del joystick vertical (saltar)
    if (!isJumping && analogRead(VERT_PIN) == 0) {
      lcd.setCursor(xPlayerPos, yPlayerPos);
      lcd.print(' ');
      yPlayerPos = 0;
      isJumping = true;
      previousTimeJump = currentTime;
    } else if (isJumping && currentTime - previousTimeJump > jumpInterval) {
      previousTimeJump = currentTime;
      lcd.setCursor(xPlayerPos, yPlayerPos);
      lcd.print(' ');
      yPlayerPos = 1;
      isJumping = false;
    }

    // Leer el estado del joystick horizontal (moverse a la derecha)
    if (analogRead(HORZ_PIN) == 4095) {
      if (xPlayerPos < 15) {
        lcd.setCursor(xPlayerPos, yPlayerPos);
        lcd.print(' ');
        xPlayerPos++;
        lcd.setCursor(xPlayerPos, yPlayerPos);
        lcd.write(PLAYER_RUNNING_RIGHT);
        playerDirection = RIGHT;
      }
    }
    // Leer el estado del joystick horizontal (moverse a la izquierda)
    else if (analogRead(HORZ_PIN) == 0) {
      if (xPlayerPos > 0) {
        lcd.setCursor(xPlayerPos, yPlayerPos);
        lcd.print(' ');
        xPlayerPos--;
        lcd.setCursor(xPlayerPos, yPlayerPos);
        lcd.write(PLAYER_RUNNING_LEFT);
        playerDirection = LEFT;
      }
    } else if (playerDirection == RIGHT) {
      lcd.setCursor(xPlayerPos, yPlayerPos);
      lcd.write(PLAYER_RIGHT);
    } else {
      lcd.setCursor(xPlayerPos, yPlayerPos);
      lcd.write(PLAYER_LEFT);
    }
    checkCollisions();
  }
}

void updateObstacles() {
  if (currentTime - previousTimeObstacle > obstacleInterval) {
    previousTimeObstacle = currentTime;

    // Borrar las posiciones anteriores de los obstáculos
    lcd.setCursor(xObstacleLowerPos, 1);
    lcd.print(' ');
    lcd.setCursor(xObstacleUpperPos, 0);
    lcd.print(' ');

    // Mover obstáculo inferior
    if (directionLower == LEFT) {
      if (xObstacleLowerPos > 0) {
        xObstacleLowerPos--;
      } else {
        directionLower = RIGHT;
        xObstacleLowerPos = 0;
      }
    } else {
      if (xObstacleLowerPos < 15) {
        xObstacleLowerPos++;
      } else {
        directionLower = LEFT;
        xObstacleLowerPos = 15;
      }
    }

    // Mover obstáculo superior
    if (directionUpper == LEFT) {
      if (xObstacleUpperPos > 0) {
        xObstacleUpperPos--;
      } else {
        directionUpper = RIGHT;
        xObstacleUpperPos = 0;
      }
    } else {
      if (xObstacleUpperPos < 15) {
        xObstacleUpperPos++;
      } else {
        directionUpper = LEFT;
        xObstacleUpperPos = 15;
      }
    }

    // Dibujar los obstáculos en las nuevas posiciones
    lcd.setCursor(xObstacleLowerPos, 1);
    lcd.write(directionLower == LEFT ? OBSTACLE_LEFT : OBSTACLE_RIGHT);
    lcd.setCursor(xObstacleUpperPos, 0);
    lcd.write(directionUpper == LEFT ? OBSTACLE_LEFT : OBSTACLE_RIGHT);

    // Si el obstáculo pasó sobre la moneda, redibujar la moneda
    drawCoin();

    checkCollisions();
  }
}

void checkCollisions() {
  if ((xObstacleLowerPos == xPlayerPos && yPlayerPos == 1) || 
      (xObstacleUpperPos == xPlayerPos && yPlayerPos == 0)) {
    isGameOver = true;
    isGameRunning = false;
  }
  checkCoinCollision();
}

void initializeScores() {
  if (!SD.exists(SCORES_FILE)) {
    DynamicJsonDocument doc(256);
    JsonArray scoresArray = doc.createNestedArray("scores");
    for (int i = 0; i < MAX_SCORES; i++) {
      scoresArray.add(0);
    }
    File file = SD.open(SCORES_FILE, FILE_WRITE);
    if (file) {
      serializeJson(doc, file);
      file.close();
    }
  }
}

void loadScores() {
  File file = SD.open(SCORES_FILE);
  if (file) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, file);
    if (!error) {
      JsonArray scoresArray = doc["scores"];
      for (int i = 0; i < MAX_SCORES; i++) {
        scores[i] = scoresArray[i];
      }
    }
    file.close();
  }
}

void saveScore(int newScore) {
  // Insertar el nuevo puntaje en el array y ordenar
  for (int i = 0; i < MAX_SCORES; i++) {
    if (newScore > scores[i]) {
      for (int j =  MAX_SCORES-1; j > i; j--) {
        scores[j] = scores[j - 1];
      }
      scores[i] = newScore;
      break;
    }
  }
  saveScoresToSD();
}

void saveScoresToSD() {
  DynamicJsonDocument doc(256);
  JsonArray scoresArray = doc.createNestedArray("scores");
  for (int i = 0; i < MAX_SCORES; i++) {
    scoresArray.add(scores[i]);
  }
  File file = SD.open(SCORES_FILE, FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
  }
}

// Función para generar la moneda en una posición aleatoria
void spawnCoin() {
  bool coinSafePosition = false;
  while (!coinSafePosition) {
    xCoinPos = random(0, 16); // Generar posición X aleatoria
    yCoinPos = random(0, 2);  // Generar posición Y (0 o 1)
    
    // Verificar si la posición de la moneda no choca con los obstáculos
    if ((xCoinPos != xObstacleLowerPos || yCoinPos != 1) && 
        (xCoinPos != xObstacleUpperPos || yCoinPos != 0) && 
        (xPlayerPos != xCoinPos || yPlayerPos != yCoinPos)) {
      coinSafePosition = true;
    }
  }
  drawCoin(); // Dibujar la moneda en la pantalla
}

// Función para dibujar la moneda en la pantalla
void drawCoin() {
  lcd.setCursor(xCoinPos, yCoinPos);
  lcd.write(COIN); // Mostrar la moneda en la pantalla
}

// Función para verificar si el jugador ha tocado la moneda
void checkCoinCollision() {
  if (xPlayerPos == xCoinPos && yPlayerPos == yCoinPos) {
    // El jugador ha tocado la moneda
    score++; // Incrementar el puntaje

    // Borrar la moneda de la pantalla
    lcd.setCursor(xCoinPos, yCoinPos);
    lcd.print(' ');

    // Generar una nueva moneda en una posición aleatoria
    spawnCoin();

    // Mostrar el puntaje actualizado en el monitor serial
    Serial.print("Score: ");
    Serial.println(score);
  }
}

bool checkCollision() {
  if ((xObstacleLowerPos == xPlayerPos && yPlayerPos == 1) || 
      (xObstacleUpperPos == xPlayerPos && yPlayerPos == 0)) {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("GAME OVER");
    isGameOver = true;
    isGameRunning = false;
    return isGameOver;
  }
  return false;
}

// Función para guardar los puntajes en un archivo JSON

/*~~~~~~~~~~~~~~~~~~~~~~ Audio ~~~~~~~~~~~~~~~~~~~~~~~*/
void Loop2(void * pvParameters) {
  const TickType_t xDelay = pdMS_TO_TICKS(10);
  bool gameOverSoundPlayed = false;
  unsigned long gameOverStartTime = 0;
  const unsigned long gameOverDuration = 2000; // Duración aprox del archivo gameover.mp3 en milisegundos
  bool introPlayed = false;

  while (true) {
    if (isGameOver) {
      if (!gameOverSoundPlayed) {
        if (audio.isRunning()) {
          audio.stopSong();
        }
        Serial.println(F("Core 0: Iniciando reproducción de melodía de game over..."));
        audio.connecttoFS(SD, "/gameover.mp3");
        gameOverSoundPlayed = true;
        gameOverStartTime = millis();
      } else if (millis() - gameOverStartTime > gameOverDuration) {
        if (audio.isRunning()) {
          audio.stopSong();
          Serial.println(F("Core 0: Reproducción de game over finalizada."));
        }
      }
    } else if (isGameRunning) {
      if (audio.isRunning() && introPlayed) {
        audio.stopSong();
        Serial.println(F("Core 0: Deteniendo música de intro."));
      }
      if (!audio.isRunning()) {
        Serial.println(F("Core 0: Iniciando reproducción de melodía de gameplay..."));
        audio.connecttoFS(SD, "/gameplay.mp3");
      }
      gameOverSoundPlayed = false;
      introPlayed = false;
    } else if (isIntro) {
      if (!audio.isRunning() && !introPlayed) {
        Serial.println(F("Core 0: Iniciando reproducción de intro..."));
        audio.connecttoFS(SD, "/intro.mp3");
        introPlayed = true;
      }
    } else {
      if (audio.isRunning()) {
        audio.stopSong();
        Serial.println(F("Core 0: Reproducción detenida."));
      }
    }
    
    audio.loop();
    
    // Ceder el control brevemente
    vTaskDelay(xDelay);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~ Función Setup ~~~~~~~~~~~~~~~~~~~~~~*/
void setup ( void ) {
  Serial.begin ( BAUD_RATE );

  pinMode(SEL_PIN, INPUT_PULLUP); 
    
  lcd.init();
  
  lcd.createChar(PLAYER_RIGHT, caracter1);
  lcd.createChar(PLAYER_LEFT, caracter2);
  lcd.createChar(PLAYER_RUNNING_RIGHT, caracter3);
  lcd.createChar(PLAYER_RUNNING_LEFT, caracter4);
  lcd.createChar(OBSTACLE_RIGHT, caracter5);
  lcd.createChar(OBSTACLE_LEFT, caracter6);
  lcd.createChar(COIN, caracter7);

  lcd.backlight();

  // Set microSD Card CS as OUTPUT and set HIGH
  pinMode(SD_CS, OUTPUT);      
  digitalWrite(SD_CS, HIGH); 
  
  // Initialize SPI bus for microSD Card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Start microSD Card
  if(!SD.begin(SD_CS))
  {
    Serial.println(F("Error accessing microSD card!"));
    while(true); 
  }
  Serial.println("Tarjeta SD iniciada correctamente.");

  // Setup I2S 
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  
  // Set Volume
  audio.setVolume(20);

  initializeScores();
  loadScores();
  
  /* Pasar los loops como punteros a función */
  DCESP32.ConfigCores(Loop1, Loop2); 

  Serial.println(F( "Se han configurado correctamente los dos nucleos "));
}

void showIntro() {  
  // Mostrar el nombre del juego en la fila inferior
  lcd.setCursor(4, 1);  // Centrar el texto en la fila inferior
  lcd.print("JUMP MAN");

  // Mostrar el logotipo desplazandose
  for (int i = 0; i < 18; i++) {
    lcd.setCursor(i-3, 0);
    lcd.print(" ");
    lcd.setCursor(i-2, 0);
    lcd.write(OBSTACLE_RIGHT);
    lcd.setCursor(i-1, 0);
    lcd.print(" ");
    lcd.setCursor(i, 0);
    lcd.write(PLAYER_RUNNING_RIGHT);
    delay(500);
  }

  lcd.clear();

  // Mostrar los nombres de los creadores de dos en dos
  String nombres[] = {"Daniel Astilla", "Alejandro Barajas","Javier Gonzalez", "Daniel Ramirez", "Bruno Sanchez"};
  int numNombres = 5;
  
  for (int i = 0; i < numNombres; i += 2) {
    // Mostrar los nombres en dos filas
    lcd.setCursor(0, 0);
    lcd.print(nombres[i]);
    
    if (i + 1 < numNombres) {
      lcd.setCursor(0, 1);
      lcd.print(nombres[i + 1]);
    }
    
    delay(2000);  // Mostrar cada par de nombres durante 3 segundos
    lcd.clear();
  }

  isIntro = false;
}

void loop ( void ) {
  // No se necesita hacer nada en el loop principal
}