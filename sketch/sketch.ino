#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Arduino.h"
#include "Audio.h"
#include "C:\Users\bruno\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.0.4\libraries\SD\src\SD.h"
#include "FS.h"


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

#define PLAYER 0
#define PLAYER_CROUCHED 1
#define PLAYER_RUNNING_RIGHT 2
#define PLAYER_RUNNING_LEFT 3
#define PLAYER_JUMPING 4
#define BARRIER_DOWN 5
#define BARRIER_UP 6

#define DELAY 100

int gameOver = 0;

int xPlayerPos = 3;
int yPlayerPos = 1;
int isCrouching = 0;
int isJumping = 0;
int lives = 3;

int xBarrierPos = 15;
int xBarrierUpperPos = 22;

uint8_t caracter1[] = {
  B01110, B01010, B01110, B11111, B00100, B00100, B01010, B01010
};

uint8_t caracter2[] = {
  B00000, B00000, B01110, B01010, B11111, B00100, B01110, B10001
};

uint8_t caracter3[] = {
  B01110, B01010, B01110, B11100, B00111, B10100, B01010, B00001
};

uint8_t caracter4[] = {
  B01110, B01010, B01110, B00111, B11100, B00101, B01010, B10000
};

uint8_t caracter5[] = {
  B01110, B01010, B11111, B01110, B00100, B00100, B01010, B10001
};

uint8_t caracter6[] = {
  B00000, B01110, B10001, B10101, B10001, B01110, B00000, B00000
};

uint8_t caracter7[] = {
  B00000, B00000, B01110, B10001, B10101, B10001, B01110, B00000
};
 
LiquidCrystal_I2C lcd(0x27, 16, 2);
Audio audio;

// Tiempos para la multitarea
unsigned long previousMillisBarrier = 0;
unsigned long previousMillisPlayer = 0;
unsigned long previousMillisJump = 0;
const long interval = DELAY;

void setup() {
  Serial.begin(115200);

  pinMode(SEL_PIN, INPUT_PULLUP);
  
  lcd.init();
  
  lcd.createChar(PLAYER, caracter1);
  lcd.createChar(PLAYER_CROUCHED, caracter2);
  lcd.createChar(PLAYER_RUNNING_RIGHT, caracter3);
  lcd.createChar(PLAYER_RUNNING_LEFT, caracter4);
  lcd.createChar(PLAYER_JUMPING, caracter5);
  lcd.createChar(BARRIER_DOWN, caracter6);
  lcd.createChar(BARRIER_UP, caracter7);

  lcd.backlight();
  lcd.setCursor(xPlayerPos, yPlayerPos);
  lcd.write(PLAYER);
  Serial.println(lives);

  // Set microSD Card CS as OUTPUT and set HIGH
  pinMode(SD_CS, OUTPUT);      
  digitalWrite(SD_CS, HIGH); 
  
  // Initialize SPI bus for microSD Card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Start microSD Card
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Error accessing microSD card!"));
    while (true);
  }

  // Setup I2S 
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  
  // Set Volume
  audio.setVolume(60);
  
  // Open music file
  if (audio.connecttoFS(SD, "/frog_laugh.mp3")) {
    Serial.println("Archivo cargado exitosamente");
  } else {
    Serial.println("Error al cargar el archivo");
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Gestión de obstáculos
  if (currentMillis - previousMillisBarrier >= interval) {
    previousMillisBarrier = currentMillis;
      if ((xBarrierPos == xPlayerPos && yPlayerPos == 1) || (xBarrierUpperPos == xPlayerPos && yPlayerPos == 0)) {
    if (lives == 1) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("GAME OVER");
      audio.loop();
      gameOver = 1;
      if (gameOver) {
        if (!digitalRead(SEL_PIN)) {
          lcd.clear();
          gameOver = 0;
          lives = 3;
          Serial.println(lives);
        } else {
          return;
        }
      }
    } else {
      lives--;
      Serial.println(lives);
    }
  }
    updateBarriers();
  }

  // Gestión de movimiento del jugador
  if (currentMillis - previousMillisPlayer >= interval) {
    previousMillisPlayer = currentMillis;
    updatePlayerMovement();
  }

  // Música en loop si el juego terminó
  if (gameOver) {
    audio.loop();
  }
}

void updateBarriers() {
  // Verificar colisiones
  // checkCollision();

  if (xBarrierPos != 0) {
    lcd.setCursor(xBarrierPos, 1);
    lcd.print(' ');
    xBarrierPos--;
    lcd.setCursor(xBarrierPos, 1);
    lcd.write(BARRIER_DOWN);
  } else {
    lcd.setCursor(xBarrierPos, 1);
    lcd.print(' ');
    xBarrierPos = 15;
  }

  if (xBarrierUpperPos != 0) {
    lcd.setCursor(xBarrierUpperPos, 0);
    lcd.print(' ');
    xBarrierUpperPos--;
    lcd.setCursor(xBarrierUpperPos, 0);
    lcd.write(BARRIER_UP);
  } else {
    lcd.setCursor(xBarrierUpperPos, 0);
    lcd.print(' ');
    xBarrierUpperPos = 15;
  }

  lcd.setCursor(15, 0);
  lcd.print(lives);
}

void updatePlayerMovement() {
  if (!isCrouching && analogRead(VERT_PIN) == 0) {
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.write(PLAYER_CROUCHED);
    isCrouching = 1;
  } else if (isCrouching) {
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.write(PLAYER);
    isCrouching = 0;
  }

  if (!isJumping && analogRead(VERT_PIN) == 4095) {
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.print(' ');
    yPlayerPos = 0;
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.write(PLAYER_JUMPING);
    isJumping = 1;
  } else if (isJumping) {
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.print(' ');
    yPlayerPos = 1;
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.write(PLAYER);
    isJumping = 0;
  }

  if (analogRead(HORZ_PIN) == 0 && xPlayerPos < 15) {
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.print(' ');
    xPlayerPos++;
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.write(PLAYER_RUNNING_RIGHT);
  }

  if (analogRead(HORZ_PIN) == 4095 && xPlayerPos > 0) {
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.print(' ');
    xPlayerPos--;
    lcd.setCursor(xPlayerPos, yPlayerPos);
    lcd.write(PLAYER_RUNNING_LEFT);
  }
}

void checkCollision() {
  if ((xBarrierPos == xPlayerPos && yPlayerPos == 1) || (xBarrierUpperPos == xPlayerPos && yPlayerPos == 0)) {
    if (lives == 1) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("GAME OVER");
      gameOver = 1;
      if (gameOver) {
        if (!digitalRead(SEL_PIN)) {
          lcd.clear();
          gameOver = 0;
          lives = 3;
          Serial.println(lives);
        } else {
          return;
        }
      }
    } else {
      lives--;
      Serial.println(lives);
    }
  }
}