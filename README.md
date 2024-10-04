# Juego Jump Man

## Descripción General
Jump Man es un juego interactivo en 2D desarrollado para el microcontrolador ESP32. El juego utiliza una arquitectura de doble núcleo, una pantalla LCD, entrada de joystick y capacidades de reproducción de audio.

## Requisitos de Hardware
- Microcontrolador ESP32
- Pantalla LCD (interfaz I2C)
- Joystick
- Lector de tarjetas microSD
- Salida de audio I2S

## Descripción del Juego
En Jump Man, los jugadores controlan un personaje que debe evitar obstáculos y recolectar monedas. El juego presenta un entorno de desplazamiento lateral mostrado en una pantalla LCD de 16x2.

## Estructura del Código

### Componentes Principales
1. **Lógica del Juego (Núcleo 1)**
   - Maneja los estados del juego (introducción, menú, gameplay, pausa, game over)
   - Actualiza la posición del jugador y los obstáculos
   - Gestiona la puntuación y las colisiones

2. **Reproducción de Audio (Núcleo 2)**
   - Maneja la música de fondo y los efectos de sonido
   - Cambia el audio basado en el estado del juego

3. **Configuración**
   - Inicializa los componentes de hardware
   - Configura la configuración de doble núcleo

### Funciones Clave
- `Loop1`: Bucle principal del juego (Núcleo 1)
- `Loop2`: Bucle de gestión de audio (Núcleo 2)
- `updatePlayer`: Maneja el movimiento y salto del jugador
- `updateObstacles`: Gestiona el movimiento de los obstáculos
- `checkCollisions`: Detecta colisiones entre el jugador, obstáculos y monedas
- `spawnCoin`: Genera monedas en posiciones aleatorias
- `saveScore`: Gestiona el seguimiento de puntuaciones altas

## Características del Juego
- Movimiento del personaje (izquierda/derecha)
- Mecanismo de salto
- Evitación de obstáculos
- Recolección de monedas
- Seguimiento de puntuación
- Sistema de puntuaciones altas (almacenado en tarjeta SD)
- Funcionalidad de pausa
- Estado de game over
- Sistema de menú (Iniciar juego, Ver puntuaciones altas)

## Audio
- Música de introducción
- Música de fondo para el gameplay
- Efecto de sonido de game over

## Caracteres Personalizados
El juego utiliza caracteres personalizados para el jugador, obstáculos y monedas, creados utilizando la función de caracteres personalizados del LCD.

## Gestión de Archivos
- Las puntuaciones se guardan en un archivo JSON en la tarjeta SD
- Los archivos de audio (intro.mp3, gameplay.mp3, gameover.mp3) se almacenan en la tarjeta SD

## Configuración y Compilación
1. Asegúrate de que todas las bibliotecas necesarias estén instaladas (DualCore, Wire, LiquidCrystal_I2C, Audio, SD, ArduinoJson)
2. Conecta los componentes de hardware según se especifica en las definiciones de pines
3. Sube el código a tu placa ESP32
4. Asegúrate de que todos los archivos de audio y el archivo JSON de puntuaciones estén presentes en la tarjeta SD

## Controles
- Usa el joystick para moverte a la izquierda/derecha
- Tira del joystick hacia arriba para saltar
- Presiona el botón del joystick para pausar/reanudar el juego

¡Disfruta jugando a Jump Man!
