#include "mbed.h"
#include "storage/blockdevice/COMPONENT_SPIF/include/SPIF/SPIFBlockDevice.h"
#include "HX711.h"
#include "GlobalDefines.h"
#include "USBSerial.h"
#include "mbed_events.h"
#include "EventQueue.h"
#include <chrono>
#include <cstdio>

using namespace std::chrono;

// Seção USB Serial
static constexpr auto UsbSerialBlocking = false;
static bool isConsoleInitialised = false;

class MyConsole : public USBSerial {
    using USBSerial::USBSerial;

    virtual int _putc(int c) override {
        USBSerial::_putc(c);
        return c;
    }
};

static USBSerial* get_console() {
    static MyConsole console(UsbSerialBlocking);

    if (!isConsoleInitialised) {
        console.init();
        while (!console.connected()) {
            ThisThread::sleep_for(100ms);
        }
        printf("USB Serial Conectado!\r\n");
        isConsoleInitialised = true;
    }

    return &console;
}

namespace mbed {
    FileHandle *mbed_override_console(int fd) {
        return get_console();
    }
}

// --- Configurações Gerais ---
#define BUFFER_SIZE 128  // Tamanho do buffer para armazenar dados temporariamente
#define CELL_ID 0x01     // Identificador para a célula de carga
#define PRESSURE_ID 0x02 // Identificador para o transdutor de pressão

// --- Pinos ---
#define PIN_DT PB_5
#define PIN_SCK PB_4
#define PRESSURE_PIN PA_0 // Pino analógico para o transdutor

// --- Variáveis Globais ---
HX711 loadcell;
uint32_t lastCellRead = 0;     // Tempo da última leitura da célula de carga
uint32_t lastPressureRead = 0; // Tempo da última leitura do transdutor
float escala = -3940;
Timer globalTimer;

// Buffer para armazenamento de dados temporário
struct DataBlock {
  uint8_t id;     // Identificador do sensor
  uint32_t time;  // Timestamp da leitura
  float value;    // Valor medido
} buffer[BUFFER_SIZE];

volatile int bufferIndex = 0; // Índice atual do buffer

// --- Configurações de Timer ---
#define CELL_SAMPLE_INTERVAL 12ms  // Intervalo entre leituras da célula (80 Hz = 1000 ms / 80)
#define PRESSURE_SAMPLE_INTERVAL 20ms // Intervalo entre leituras do transdutor (50 Hz = 1000 ms / 50)

// --- Flash Configuration ---
SPIFBlockDevice flash(FLASH_MOSI, FLASH_MISO, FLASH_SCK, FLASH_CS);
#define START_ADDRESS 0x00000000  // Endereço de início na memória flash
#define FLOAT_SIZE sizeof(float)

// --- Transducer Configuration ---
AnalogIn analog_pin(PA_0); 

// --- Mutex para proteger o buffer compartilhado ---
Mutex bufferMutex;

Thread thread1;     // Declaração de threads
Thread thread2;

void inicializa_loadcell() {
    printf("Inicializando HX711...\r\n");
    loadcell.begin(PIN_DT, PIN_SCK);
    printf("Aguardando calibracao inicial...\r\n");
    ThisThread::sleep_for(3s);

    // Realiza tare para zerar leituras
    loadcell.tare();
    loadcell.set_scale(escala); // Fator de escala inicial

    printf("Calibracao concluida. Balanca zerada.\r\n");
}

// --- Função para adicionar dados ao buffer ---
void adicionarAoBuffer(uint8_t id, uint32_t time, float value) {
    bufferMutex.lock(); // Protege o buffer durante o acesso
    if (bufferIndex < BUFFER_SIZE) {
        buffer[bufferIndex].id = id;
        buffer[bufferIndex].time = time;
        buffer[bufferIndex].value = value;
        bufferIndex++;
    }
    bufferMutex.unlock();
}

// --- Thread para leitura do sensor ID1 ---
void threadCelulaCarga() {
    while (true) {
        uint32_t currentTime = duration_cast<milliseconds>(globalTimer.elapsed_time()).count();
        float value = loadcell.get_units(1);
        adicionarAoBuffer(CELL_ID, currentTime, value);
        ThisThread::sleep_for(CELL_SAMPLE_INTERVAL); // Aguarda 12 ms
    }
}

// --- Thread para leitura do sensor ID2 ---
void threadTransdutorPressao() {
    while (true) {
        uint32_t currentTime = duration_cast<milliseconds>(globalTimer.elapsed_time()).count();
        float value = analog_pin.read() * 10000; // Converte para unidade arbitrária
        adicionarAoBuffer(PRESSURE_ID, currentTime, value);
        ThisThread::sleep_for(PRESSURE_SAMPLE_INTERVAL); // Aguarda 20 ms
    }
}

// --- Função para salvar dados na memória flash ---
void salvarFlash() {

    printf("Salvando dados na memoria flash...\r\n");

    uint32_t addr = START_ADDRESS;

    for (int i = 0; i < bufferIndex; i++) {
        flash.init();

        float value = buffer[i].value;
        uint32_t time = buffer[i].time;
        uint8_t id = buffer[i].id;

        // Grava os valores na memória flash
        flash.program(&id, addr, sizeof(uint8_t));
        addr += sizeof(uint8_t);
        flash.program(&time, addr, sizeof(uint32_t));
        addr += sizeof(uint32_t);
        flash.program(&value, addr, FLOAT_SIZE);
        addr += FLOAT_SIZE;

        flash.deinit();

        printf("ID: %d, Tempo: %u, Valor: %.2f salvo na memoria flash.\r\n", id, time, value);
    }

    printf("Gravacao concluida.\r\n");
}

int main(){
    printf("Inicializando sistema...\r\n");

    // Inicializa a flash
    int result = flash.init();
    if (result != 0) {
        printf("Falha ao inicializar a flash. Codigo de erro: %d\r\n", result);
        return -1;
    }
    printf("Memoria flash inicializada com sucesso!\r\n");

    // Inicializa célula de carga
    inicializa_loadcell();

    // Inicializa threads
    thread1.start(threadCelulaCarga);
    thread2.start(threadTransdutorPressao);// Thread para ID2 (20 ms)

    // Inicializa temporizador
    globalTimer.start();

    while(true){
        ThisThread::sleep_for(1s); // Verifica o buffer a cada 1 segundo

        bufferMutex.lock();
        if (bufferIndex >= BUFFER_SIZE) {
            salvarFlash();
            // Limpa o buffer reiniciando o índice
            bufferIndex = 0;  
        }
        bufferMutex.unlock();
    }
}