#ifndef DualCore_h
#define DualCore_h

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  

#define NUCLEO_SECUNDARIO 0X00     // Núcleo secundario.
#define NUCLEO_PRINCIPAL  0X01     // Núcleo principalmente utilizado por el IDE en el loop original.

/* INSTANCIAS FREE-RTOS */
TaskHandle_t Hilo1;
TaskHandle_t Hilo2;

/* Tipo de puntero a función para las tareas */
typedef void (*TaskFunction_t)(void *);

class DualCoreESP32 {

  public:
    void ConfigCores(TaskFunction_t loop1, TaskFunction_t loop2);
};

void DualCoreESP32::ConfigCores(TaskFunction_t loop1, TaskFunction_t loop2) {

  xTaskCreatePinnedToCore(
    loop1,                /* Función que controla la tarea. */
    "Ciclo1",             /* Etiqueta de la tarea.          */
    10000,                /* Tamaño en memoria RAM.         */
    NULL,                 /* Parámetros de la tarea.        */
    1,                    /* Prioridad de la tarea.         */
    &Hilo1,               /* Seguimiento de la tarea.       */
    NUCLEO_PRINCIPAL      /* Número de núcleo.              */
  );

  xTaskCreatePinnedToCore(
    loop2,                /* Función que controla la tarea. */
    "Ciclo2",             /* Etiqueta de la tarea.          */
    10000,                /* Tamaño en memoria RAM.         */
    NULL,                 /* Parámetros de la tarea.        */
    0,                    /* Prioridad de la tarea.         */
    &Hilo2,               /* Seguimiento de la tarea.       */
    NUCLEO_SECUNDARIO     /* Número de núcleo.              */
  );
}

#endif
