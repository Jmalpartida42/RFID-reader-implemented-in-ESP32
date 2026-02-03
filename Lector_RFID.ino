#include <Arduino.h>
#include "FS.h"
#include "LittleFS.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURACIÓN PANTALLA ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- PINOUT ---
#define PIN_D0 18       // Wiegand D0
#define PIN_D1 19       // Wiegand D1

// --- CONFIGURACIÓN TÁCTIL (TOUCH) ---
// Pines: GPIO 13, 4, 15
#define TOUCH_PIN_UP   13
#define TOUCH_PIN_DOWN 4
#define TOUCH_PIN_SEL  15

// UMBRAL (THRESHOLD): Ajustar según tu montaje (Sin tocar ~70, Tocado ~15)
const int UMBRAL_TOUCH = 17; 

// --- CONFIGURACIÓN USUARIOS ---
const unsigned long ID_MAESTRA = 134440; 
const char* RUTA_DB = "/database.txt";
const char* RUTA_TEMP = "/temp.txt";

// --- ESTADOS DEL SISTEMA ---
enum EstadoSistema {
  NORMAL,         
  MENU_PRINCIPAL, 
  ESPERA_NUEVO,   
  ESPERA_BORRAR   
};
EstadoSistema estadoActual = NORMAL;

// --- VARIABLES WIEGAND ---
volatile unsigned long lastPulseTime = 0;
volatile int bitCount = 0;
volatile byte cardBits[100]; 
const int transmissionGap = 50; 

// --- VARIABLES MENU ---
int opcionMenu = 0; 
const char* opciones[] = {"New ID", "Delete ID", "Exit"};

// --- PROTOTIPOS ---
void IRAM_ATTR ISR_D0();
void IRAM_ATTR ISR_D1();
void logicStateNormal(unsigned long id);
void logicStateMenu();
void logicStateAdd(unsigned long id);
void logicStateDelete(unsigned long id);
void dibujarMenu();
void mostrarMensaje(String titulo, String msg, int delayTime = 0);
bool eliminarID(unsigned long idTarget);
bool leerTouch(int pin); // Función auxiliar para limpiar la lectura

void setup() {
  Serial.begin(115200);

  // Inicializar Hardware
  pinMode(PIN_D0, INPUT_PULLUP);
  pinMode(PIN_D1, INPUT_PULLUP);
  
  // NOTA: Los pines Touch NO necesitan pinMode, se configuran solos al leer.

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  display.clearDisplay();
  
  if(!LittleFS.begin(true)){
    mostrarMensaje("ERROR", "LittleFS Fail");
    return;
  }
  
  attachInterrupt(digitalPinToInterrupt(PIN_D0), ISR_D0, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_D1), ISR_D1, FALLING);

  mostrarMensaje("SISTEMA", "TOUCH LISTO", 1000);
}

void loop() {
  // 1. LECTURA WIEGAND
  unsigned long idLeido = 0;
  unsigned long currentTime = millis();
  
  if (bitCount > 0 && (currentTime - lastPulseTime > transmissionGap)) {
    noInterrupts();
    int bits = bitCount;
    byte tempBits[100];
    memcpy(tempBits, (const void*)cardBits, bits);
    bitCount = 0; 
    interrupts();

    if (bits > 20) {
      for (int i = 29; i < bits - 1; i++) idLeido = (idLeido << 1) | tempBits[i];
      Serial.print("ID Leido: "); Serial.println(idLeido);
    }
  }

  // 2. MÁQUINA DE ESTADOS
  switch (estadoActual) {
    
    case NORMAL:
      if (idLeido > 0) logicStateNormal(idLeido);
      else {
         // Opcional: Mostrar "Presione ID" o algo en pantalla
         // display.display() se evita aquí para no parpadear innecesariamente
      }
      break;

    case MENU_PRINCIPAL:
      logicStateMenu(); 
      break;

    case ESPERA_NUEVO:
      if (idLeido > 0) logicStateAdd(idLeido);
      // Cancelar con botón SELECT
      if (leerTouch(TOUCH_PIN_SEL)) {
        estadoActual = MENU_PRINCIPAL;
        dibujarMenu();
      }
      break;

    case ESPERA_BORRAR:
      if (idLeido > 0) logicStateDelete(idLeido);
      // Cancelar con botón SELECT
      if (leerTouch(TOUCH_PIN_SEL)) {
        estadoActual = MENU_PRINCIPAL;
        dibujarMenu();
      }
      break;
  }
}

// ==========================================
//      LECTURA DE TOUCH OPTIMIZADA
// ==========================================
bool leerTouch(int pin) {
  // touchRead devuelve valores BAJOS cuando se toca (< 20-30)
  // y ALTOS cuando no se toca (> 70)
  if (touchRead(pin) < UMBRAL_TOUCH) {
    delay(200); // Debounce y espera para evitar disparos múltiples
    return true;
  }
  return false;
}

// ==========================================
//      LÓGICA DE ESTADOS
// ==========================================

void logicStateNormal(unsigned long id) {
  if (id == ID_MAESTRA) {
    mostrarMensaje("ADMIN", "MASTER KEY", 1000);
    estadoActual = MENU_PRINCIPAL;
    opcionMenu = 0; 
    dibujarMenu();
  } else {
    // Validar acceso
    File f = LittleFS.open(RUTA_DB, "r");
    bool access = false;
    if (f) {
      String sID = String(id);
      while(f.available()){
        String line = f.readStringUntil('\n');
        line.trim();
        if(line == sID) { access = true; break; }
      }
      f.close();
    }
    
    if (access) mostrarMensaje("ACCESO", "PERMITIDO", 2000);
    else mostrarMensaje("ACCESO", "DENEGADO", 2000);
    
    mostrarMensaje("SISTEMA", "ACTIVO"); // Volver a pantalla base
  }
}

void logicStateMenu() {
  // Navegación Táctil
  
  if (leerTouch(TOUCH_PIN_UP)) { // ARRIBA
    opcionMenu--;
    if (opcionMenu < 0) opcionMenu = 2;
    dibujarMenu();
  }
  
  else if (leerTouch(TOUCH_PIN_DOWN)) { // ABAJO
    opcionMenu++;
    if (opcionMenu > 2) opcionMenu = 0;
    dibujarMenu();
  }

  else if (leerTouch(TOUCH_PIN_SEL)) { // SELECT
    
    if (opcionMenu == 0) { // NEW ID
      estadoActual = ESPERA_NUEVO;
      mostrarMensaje("NEW ID", "PUT TARGET");
    } 
    else if (opcionMenu == 1) { // DELETE ID
      estadoActual = ESPERA_BORRAR;
      mostrarMensaje("DELETE ID", "PUT TARGET");
    } 
    else { // EXIT
      estadoActual = NORMAL;
      mostrarMensaje("EXIT", "BYE BYE...", 1000);
      mostrarMensaje("SISTEMA", "ACTIVO");
    }
  }
}

void logicStateAdd(unsigned long id) {
  if(id == ID_MAESTRA) return;
  
  File f = LittleFS.open(RUTA_DB, "a");
  if (f) {
    f.println(id);
    f.close();
    mostrarMensaje("SUCCESS", "NEW ID SAVED", 2000);
  } else {
    mostrarMensaje("ERROR", "FS WRITE", 2000);
  }
  estadoActual = MENU_PRINCIPAL;
  dibujarMenu();
}

void logicStateDelete(unsigned long id) {
  if(id == ID_MAESTRA) {
    mostrarMensaje("ERROR", "CANT DEL MASTER", 1500);
    return;
  }
  
  mostrarMensaje("WAIT", "DELETING...");
  
  if (eliminarID(id)) {
    mostrarMensaje("SUCCESS", "ID DELETED", 2000);
  } else {
    mostrarMensaje("INFO", "ID NOT FOUND", 2000);
  }
  estadoActual = MENU_PRINCIPAL;
  dibujarMenu();
}

// ==========================================
//      UTILIDADES
// ==========================================

void dibujarMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("--- TOUCH MENU ---")); // Cambié el título
  
  for(int i=0; i<3; i++) {
    display.setCursor(10, 15 + (i*15));
    if(i == opcionMenu) display.print(">"); 
    else display.print(" ");
    display.println(opciones[i]);
  }
  display.display();
}

void mostrarMensaje(String titulo, String msg, int delayTime) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(titulo);
  display.setTextSize(1);
  display.setCursor(0,25);
  display.println(msg);
  display.display();
  if(delayTime > 0) delay(delayTime);
}

bool eliminarID(unsigned long idTarget) {
  String sTarget = String(idTarget);
  bool encontrado = false;
  
  File fOrig = LittleFS.open(RUTA_DB, "r");
  File fTemp = LittleFS.open(RUTA_TEMP, "w"); 
  
  if (!fOrig || !fTemp) return false;

  while(fOrig.available()) {
    String linea = fOrig.readStringUntil('\n');
    String lineaLimpia = linea;
    lineaLimpia.trim();
    
    if (lineaLimpia == sTarget) encontrado = true; 
    else fTemp.println(linea); 
  }
  
  fOrig.close();
  fTemp.close();
  
  LittleFS.remove(RUTA_DB);
  LittleFS.rename(RUTA_TEMP, RUTA_DB);
  
  return encontrado;
}

void IRAM_ATTR ISR_D0() { lastPulseTime = millis(); if(bitCount<100) cardBits[bitCount++] = 0; }
void IRAM_ATTR ISR_D1() { lastPulseTime = millis(); if(bitCount<100) cardBits[bitCount++] = 1; }