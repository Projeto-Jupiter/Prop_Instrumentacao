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

// Buffer para armazenamento de dados temporário
struct DataBlock {
  uint8_t id;     // Identificador do sensor
  uint32_t time;  // Timestamp da leitura
  float value;    // Valor medido
} buffer[BUFFER_SIZE];

volatile int bufferIndex = 0; // Índice atual do buffer

// --- Configurações de Timer ---
#define CELL_SAMPLE_INTERVAL 12  // Intervalo entre leituras da célula (80 Hz = 1000 ms / 80)
#define PRESSURE_SAMPLE_INTERVAL 20 // Intervalo entre leituras do transdutor (50 Hz = 1000 ms / 50)

// --- Flash Configuration ---
SPIFBlockDevice flash(FLASH_MOSI, FLASH_MISO, FLASH_SCK, FLASH_CS);
#define START_ADDRESS 0x00000000  // Endereço de início na memória flash
#define FLOAT_SIZE sizeof(float)

// --- Transducer Configuration ---
AnalogIn analog_pin(PA_0); 

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

    if (bufferIndex < BUFFER_SIZE) {
        buffer[bufferIndex].id = id;
        buffer[bufferIndex].time = time;
        buffer[bufferIndex].value = value;
        bufferIndex++;
    }
}

// --- Rotina para leitura da célula de carga e do transdutor ---
void salvarDados() {
    printf("Entrou no salvamento\r\n");
    uint32_t currentMillis = duration_cast<milliseconds>(Kernel::Clock::now().time_since_epoch()).count();

    // Leitura da célula de carga
    if (currentMillis - lastCellRead >= CELL_SAMPLE_INTERVAL) {
        // if (loadcell.is_ready()) { //Remover isso 
        float cellValue = loadcell.get_units();
        adicionarAoBuffer(CELL_ID, currentMillis, cellValue);
        // }
        lastCellRead = currentMillis;
    }

    // Leitura do transdutor de pressão
    if (currentMillis - lastPressureRead >= PRESSURE_SAMPLE_INTERVAL) {
        float pressureValue = analog_pin.read();
        uint16_t analog_12bit = pressureValue * 4095; //Comentar para o transdutor (para LDR apenas)

        adicionarAoBuffer(PRESSURE_ID, currentMillis, analog_12bit); //Para o LDR
        // adicionarAoBuffer(PRESSURE_ID, currentMillis, pressureValue); //Descomentar para o transdutor
        lastPressureRead = currentMillis;
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

    printf("Gravação concluida.\r\n");
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

    while(true){
        salvarDados();
        // Verifica se o buffer está cheio
        if (bufferIndex >= BUFFER_SIZE) {
            printf("Passou pelo buffer\r\n");
            salvarFlash();
            bufferIndex = 0; // Reinicia o índice
        }
        // printf("Não entrou no if buffer\r\n");
    }
}