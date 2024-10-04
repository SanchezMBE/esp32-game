/*~~~~~~~~~~~~~~~~~~~~~~ Directivas ~~~~~~~~~~~~~~~~~~~~~~*/
#define BAUD_RATE 115200 // Tasa de baudios para la comunicación serial

#include "DualCore.h"           // Manejo de doble núcleo en ESP32
#include <Wire.h>               // Comunicación I2C
#include <LiquidCrystal_I2C.h>  // Control de la pantalla LCD I2C
#include "Arduino.h"            
#include "Audio.h"              // Manejo de audio
#include "C:\Users\bruno\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.0.4\libraries\SD\src\SD.h"
#include "FS.h"
#include <ArduinoJson.h>         // Manejo de JSON para los puntajes

// Conexiones del Joystick
#define HORZ_PIN     34
#define VERT_PIN     35
#define SEL_PIN      17

// Conexiones del lector de tarjetas microSD
#define SD_CS          5
#define SPI_MOSI      23 
#define SPI_MISO      19
#define SPI_SCK       18

// Conexiones I2S para salida de audio
#define I2S_DOUT     25  // Salida de datos
#define I2S_BCLK     27  // Reloj de bits
#define I2S_LRC      26   // Selección de canal

// Identificadores de los personajes y objetos
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
const char* SCORES_FILE = "/scores.json"; // Archivo JSON para puntajes
const int MAX_SCORES = 4;

// Estados del juego
bool isIntro = true;
bool isMenu = false;
bool isGameRunning = false;
bool isGameOver = false;

// Variables del juego
int scores[4] = {0}; // Array para almacenar los puntajes (máximo 4)
int menuSelectedOption = 0;
bool isPaused = false;
int selectedOption = 0; // Opciones de juego (0 = izquierda, 1 = derecha)

// Dirección del movimiento
enum Direction {
  LEFT, // moverse a la izquierda
  RIGHT // moverse a la derecha
};

int xPlayerPos = 3, int yPlayerPos = 1; //Posicion del jugador
bool isJumping = false;
Direction playerDirection = RIGHT;

int xObstacleLowerPos = 15, int xObstacleUpperPos = 22; // Posición de los obstáculos
Direction directionLower = LEFT;
Direction directionUpper = LEFT;

int score = 0;  // Puntaje actual
uint8_t scoreDigits;
int xCoinPos, yCoinPos;  // Posición de la moneda

// Caracteres personalizados para la pantalla LCD
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

/*~~~~~~~~~~~~~~~~~~~~~~ Game ~~~~~~~~~~~~~~~~~~~~~~~*/
// Función principal del juego que se ejecuta en el núcleo 1
void Loop1(void *pvParameters) {
  showIntro();  // Muestra la introducción del juego
  isGameRunning = true;

  for (;;) {  // Bucle infinito para el juego
    currentTime = millis();  // Obtiene el tiempo actual para la sincronización

    if (isGameOver) {
      handleGameOver(selectedOption);  // Maneja la pantalla de fin de juego
    } else if (isPaused) {
      handlePauseMenu(selectedOption, isPaused);  // Maneja el menú de pausa
    } else if (isMenu) {
      handleMenu(selectedOption);  // Maneja el menú principal
    } else if (isGameRunning) {
      // Verifica si se presiona el botón de pausa
      if (digitalRead(SEL_PIN) == LOW) {
        isPaused = true;
        selectedOption = 0;
        showPauseMenu(selectedOption);
        delay(200);  // Debounce para evitar múltiples detecciones
        continue;
      }

      // Actualiza los elementos del juego
      updatePlayer();
      updateObstacles();
      saveScore(score);  // Guarda el puntaje actual
    }
  }
}

// Maneja la pantalla de fin de juego y las opciones disponibles
void handleGameOver(int &selectedOption) {
  static bool optionsShown = false;

  if (!optionsShown) {
    showGameOverOptions(selectedOption);
    optionsShown = true;
  }

  // Maneja la selección de opciones con el joystick
  if (analogRead(HORZ_PIN) == 4095) {  // Joystick movido a la derecha
    selectedOption = 1;
    showGameOverOptions(selectedOption);
  } else if (analogRead(HORZ_PIN) == 0) {  // Joystick movido a la izquierda
    selectedOption = 0;
    showGameOverOptions(selectedOption);
  }

  // Procesa la selección cuando se presiona el botón
  if (digitalRead(SEL_PIN) == LOW) {
    if (selectedOption == 0) {
      resetGame();  // Reinicia el juego
      isGameOver = false;
      isGameRunning = true;
    } else {
      goToMenu();  // Vuelve al menú principal
    }
    optionsShown = false;
    delay(200);  // Debounce para evitar múltiples detecciones
  }
}

// Maneja el menú de pausa y las opciones disponibles
void handlePauseMenu(int &selectedOption, bool &isPaused) {
  // Maneja la selección de opciones con el joystick
  if (analogRead(HORZ_PIN) == 4095) {  // Joystick movido a la derecha
    selectedOption = 1;
    showPauseMenu(selectedOption);
  } else if (analogRead(HORZ_PIN) == 0) {  // Joystick movido a la izquierda
    selectedOption = 0;
    showPauseMenu(selectedOption);
  }

  // Procesa la selección cuando se presiona el botón
  if (digitalRead(SEL_PIN) == LOW) {
    if (selectedOption == 0) {
      isPaused = false;
      lcd.clear();
      redrawGameState();  // Redibuja el estado del juego al reanudar
    } else {
      goToMenu();  // Vuelve al menú principal
    }
    delay(200);  // Debounce para evitar múltiples detecciones
  }
}

// Muestra las opciones de fin de juego en la pantalla LCD
void showGameOverOptions(int selectedOption) {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("GAME OVER");
  lcd.setCursor(0, 1);
  lcd.print(selectedOption == 0 ? ">" : " ");  // Muestra un cursor para la opción seleccionada
  lcd.print("Reiniciar");
  lcd.setCursor(11, 1);
  lcd.print(selectedOption == 1 ? ">" : " ");
  lcd.print("Menu");
}

// Muestra el menú de pausa en la pantalla LCD
void showPauseMenu(int selectedOption) {
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("PAUSA");
  lcd.setCursor(0, 1);
  lcd.print(selectedOption == 0 ? ">" : " ");  // Muestra un cursor para la opción seleccionada
  lcd.print("Reanudar");
  lcd.setCursor(10, 1);
  lcd.print(selectedOption == 1 ? ">" : " ");
  lcd.print("Menu");
}

// Cambia al menú principal
void goToMenu() {
  isGameRunning = false;
  isGameOver = false;
  isPaused = false;
  isMenu = true;
  selectedOption = 0;
  showMenu(selectedOption);
}

// Muestra el menú principal en la pantalla LCD
void showMenu(int selectedOption) {
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("MENU");
  lcd.setCursor(0, 1);
  lcd.print(selectedOption == 0 ? ">" : " ");  // Muestra un cursor para la opción seleccionada
  lcd.print("Iniciar");
  lcd.setCursor(9, 1);
  lcd.print(selectedOption == 1 ? ">" : " ");
  lcd.print("Scores");
}

// Maneja la interacción con el menú principal
void handleMenu(int &selectedOption) {
  // Maneja la selección de opciones con el joystick
  if (analogRead(HORZ_PIN) == 4095) {  // Joystick movido a la derecha
    selectedOption = 1;
    showMenu(selectedOption);
  } else if (analogRead(HORZ_PIN) == 0) {  // Joystick movido a la izquierda
    selectedOption = 0;
    showMenu(selectedOption);
  }

  // Procesa la selección cuando se presiona el botón
  if (digitalRead(SEL_PIN) == LOW) {
    if (selectedOption == 0) {
      isMenu = false;
      isGameRunning = true;
      resetGame();  // Inicia un nuevo juego
    } else {
      showScores();  // Muestra la pantalla de puntajes
    }
    delay(200);  // Debounce para evitar múltiples detecciones
  }
}

// Muestra los puntajes más altos en la pantalla LCD
void showScores() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scores:");
  for (int i = 0; i < 4; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(i + 1);
    lcd.print(". Puntaje: ");
    lcd.print(scores[i]);
    delay(2000);  // Muestra cada puntaje durante 2 segundos
  }
  lcd.clear();
  showMenu(selectedOption);  // Vuelve al menú principal después de mostrar los puntajes
}

// Reinicia todas las variables del juego a sus valores iniciales
void resetGame() {
  xPlayerPos = 3;
  yPlayerPos = 1;
  isJumping = false;
  xObstacleLowerPos = 15;
  xObstacleUpperPos = 22;
  directionLower = LEFT;
  directionUpper = LEFT;
  score = 0;
  spawnCoin();  // Genera una nueva moneda
  lcd.clear();
}

// Redibuja todos los elementos del juego en la pantalla LCD
void redrawGameState() {
  // Dibuja al jugador
  lcd.setCursor(xPlayerPos, yPlayerPos);
  lcd.write(playerDirection == RIGHT ? PLAYER_RIGHT : PLAYER_LEFT);
  
  // Dibuja los obstáculos
  lcd.setCursor(xObstacleLowerPos, 1);
  lcd.write(directionLower == LEFT ? OBSTACLE_LEFT : OBSTACLE_RIGHT);
  lcd.setCursor(xObstacleUpperPos, 0);
  lcd.write(directionUpper == LEFT ? OBSTACLE_LEFT : OBSTACLE_RIGHT);
  
  drawCoin();  // Dibuja la moneda
  
  // Muestra el puntaje actual
  scoreDigits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;
  lcd.setCursor(16 - scoreDigits, 0);
  lcd.print(score);
}

// Actualiza la posición y estado del jugador
void updatePlayer() {
  if (currentTime - previousTimePlayer > playerMoveInterval) {
    previousTimePlayer = currentTime;

    // Maneja el salto del jugador
    if (!isJumping && analogRead(VERT_PIN) == 0) {  // Joystick movido hacia arriba
      lcd.setCursor(xPlayerPos, yPlayerPos);
      lcd.print(' ');  // Borra la posición anterior
      yPlayerPos = 0;  // Mueve al jugador a la fila superior
      isJumping = true;
      previousTimeJump = currentTime;
    } else if (isJumping && currentTime - previousTimeJump > jumpInterval) {
      previousTimeJump = currentTime;
      lcd.setCursor(xPlayerPos, yPlayerPos);
      lcd.print(' ');  // Borra la posición anterior
      yPlayerPos = 1;  // Devuelve al jugador a la fila inferior
      isJumping = false;
    }

    // Maneja el movimiento horizontal del jugador
    if (analogRead(HORZ_PIN) == 4095) {  // Joystick movido a la derecha
      if (xPlayerPos < 15) {
        lcd.setCursor(xPlayerPos, yPlayerPos);
        lcd.print(' ');  // Borra la posición anterior
        xPlayerPos++;
        lcd.setCursor(xPlayerPos, yPlayerPos);
        lcd.write(PLAYER_RUNNING_RIGHT);
        playerDirection = RIGHT;
      }
    } else if (analogRead(HORZ_PIN) == 0) {  // Joystick movido a la izquierda
      if (xPlayerPos > 0) {
        lcd.setCursor(xPlayerPos, yPlayerPos);
        lcd.print(' ');  // Borra la posición anterior
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
    checkCollisions();  // Verifica colisiones después de mover al jugador
  }
}

// Actualiza la posición de los obstáculos
void updateObstacles() {
  if (currentTime - previousTimeObstacle > obstacleInterval) {
    previousTimeObstacle = currentTime;

    // Borra las posiciones anteriores de los obstáculos
    lcd.setCursor(xObstacleLowerPos, 1);
    lcd.print(' ');
    lcd.setCursor(xObstacleUpperPos, 0);
    lcd.print(' ');

    // Mueve el obstáculo inferior
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

    // Mueve el obstáculo superior
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

    // Dibuja los obstáculos en sus nuevas posiciones
    lcd.setCursor(xObstacleLowerPos, 1);
    lcd.write(directionLower == LEFT ? OBSTACLE_LEFT : OBSTACLE_RIGHT);
    lcd.setCursor(xObstacleUpperPos, 0);
    lcd.write(directionUpper == LEFT ? OBSTACLE_LEFT : OBSTACLE_RIGHT);

    drawCoin();  // Asegura que la moneda siga visible
    checkCollisions();  // Verifica colisiones después de mover los obstáculos
  }
}

// Verifica colisiones entre el jugador, los obstáculos y la moneda
void checkCollisions() {
  if ((xObstacleLowerPos == xPlayerPos && yPlayerPos == 1) || 
      (xObstacleUpperPos == xPlayerPos && yPlayerPos == 0)) {
    isGameOver = true;
    isGameRunning = false;
  }
  checkCoinCollision();  // Verifica si el jugador ha recogido la moneda
}

// Inicializa los puntajes en la tarjeta SD si no existen
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

// Carga los puntajes desde la tarjeta SD
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

// Guarda un nuevo puntaje si es lo suficientemente alto
void saveScore(int newScore) {
  // Insertar el nuevo puntaje en el array y ordenar
  bool inserted = false;  // Variable para indicar si el puntaje ha sido insertado
  for (int i = 0; i < MAX_SCORES; i++) {
    if (newScore > scores[i] && !inserted) {  // Si el nuevo puntaje es mayor y aún no ha sido insertado
      // Desplazar puntajes hacia abajo
      for (int j = MAX_SCORES - 1; j > i; j--) {
        scores[j] = scores[j - 1];
      }
      scores[i] = newScore;  // Insertar el nuevo puntaje en la posición correcta
      inserted = true;  // Marcar que el puntaje ha sido insertado
      break;  // Salir del bucle una vez que el nuevo puntaje ha sido insertado
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

// Verifica colisión del jugador con los obstáculos
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

/*~~~~~~~~~~~~~~~~~~~~~~ Audio ~~~~~~~~~~~~~~~~~~~~~~~*/

// Maneja la reproducción de audio en el núcleo 2
void Loop2(void * pvParameters) {
  const TickType_t xDelay = pdMS_TO_TICKS(10);
  bool gameOverSoundPlayed = false;
  unsigned long gameOverStartTime = 0;
  const unsigned long gameOverDuration = 2000; // Duración aprox. de gameover.mp3
  bool introPlayed = false;

  while (true) {
    // Reproducción de sonido de Game Over
    if (isGameOver) {
      if (!gameOverSoundPlayed) {
        audio.stopSong();
        Serial.println(F("Core 0: Iniciando melodía de game over..."));
        audio.connecttoFS(SD, "/gameover.mp3");
        gameOverSoundPlayed = true;
        gameOverStartTime = millis();
      } else if (millis() - gameOverStartTime > gameOverDuration) {
        audio.stopSong();
        Serial.println(F("Core 0: Fin de melodía de game over."));
      }
    } 
    // Reproducción de música durante el juego
    else if (isGameRunning) {
      if (audio.isRunning() && introPlayed) {
        audio.stopSong();
        Serial.println(F("Core 0: Deteniendo música de intro."));
      }
      if (!audio.isRunning()) {
        Serial.println(F("Core 0: Iniciando música de gameplay..."));
        audio.connecttoFS(SD, "/gameplay.mp3");
      }
      gameOverSoundPlayed = false;
      introPlayed = false;
    } 
    // Reproducción de música de introducción
    else if (isIntro) {
      if (!audio.isRunning() && !introPlayed) {
        Serial.println(F("Core 0: Iniciando música de intro..."));
        audio.connecttoFS(SD, "/intro.mp3");
        introPlayed = true;
      }
    } 
    // Detener la reproducción si no estamos en ninguno de los estados anteriores
    else {
      if (audio.isRunning()) {
        audio.stopSong();
        Serial.println(F("Core 0: Reproducción detenida."));
      }
    }
    
    audio.loop();
    vTaskDelay(xDelay); // Ceder el control brevemente para permitir otras tareas
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~ Función Setup ~~~~~~~~~~~~~~~~~~~~~~*/

// Configuración inicial del juego
void setup() {
  Serial.begin(BAUD_RATE);
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

  // Configuración de la tarjeta microSD como salida 
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH); 

  // Se inicializa el bus SPI para la microSD
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  if (!SD.begin(SD_CS)) {
    Serial.println(F("Error accessing microSD card!"));
    while(true);
  }
  Serial.println("Tarjeta SD iniciada correctamente.");

  // Configuración de audio I2S
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(20);

  initializeScores();
  loadScores();

  // Configurar los núcleos
  DCESP32.ConfigCores(Loop1, Loop2);
  Serial.println(F("Se han configurado correctamente los dos nucleos"));
}

// Muestra la introducción del juego
void showIntro() {
  lcd.setCursor(4, 1);
  lcd.print("JUMP MAN");

  // Animación del logotipo
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

  // Mostrar nombres de los creadores
  String nombres[] = {"Daniel Astilla", "Alejandro Barajas", "Javier Gonzalez", "Daniel Ramirez", "Bruno Sanchez"};
  int numNombres = 5;
  
  for (int i = 0; i < numNombres; i += 2) {
    lcd.setCursor(0, 0);
    lcd.print(nombres[i]);
    
    if (i + 1 < numNombres) {
      lcd.setCursor(0, 1);
      lcd.print(nombres[i + 1]);
    }
    
    delay(2000);
    lcd.clear();
  }

  isIntro = false;
}

// Bucle principal (vacío ya que la lógica se maneja en los núcleos)
void loop() {
  // No se necesita hacer nada en el loop principal
}
}
