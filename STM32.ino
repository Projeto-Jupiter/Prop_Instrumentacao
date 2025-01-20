#include <HardwareSerial.h>

// Definição dos pinos mapeados diretamente com os nomes do STM32
#define PIN_LINHA_ABASTECIMENTO_PA0  PA0   // Ativar/Desativar Linha de Abastecimento
#define PIN_VENT_PA1                  PA1   // Fechar/Abrir Vent
#define PIN_PURGA_LINHA_PA2           PA2   // Ativar/Desativar Purga da Linha
#define PIN_SUPERCHARGE_PA3           PA3   // Ativar/Desativar Supercharge
#define PIN_QUICK_DISCONNECT_PA4      PA4   // Ativar/Desativar Quick Disconnect
#define PIN_ARM_PA5                   PA5   // Ativar/Desativar ARM
#define PIN_IGNICAO_PA6               PA6   // Ativar/Desativar Ignição
#define PIN_INJECAO_PA7               PA7   // Ativar/Desativar Injeção
#define PIN_PURGA_TANQUE_PB0          PB0   // Ativar/Desativar Purga do Tanque

// Estrutura para mapear comandos
struct Command {
  const char* code;    // Código do comando (e.g., "ATL")
  uint8_t pin;         // Pino STM32 associado (e.g., PA0)
  const char* pinName; // Nome do pino STM32 para impressão (e.g., "PA0")
  bool setHigh;        // Ação: true para HIGH, false para LOW
};

// Mapeamento dos comandos
Command commandMap[] = {
  {"ATL", PIN_LINHA_ABASTECIMENTO_PA0, "PA0", true},   // Ativar Linha de Abastecimento
  {"DTL", PIN_LINHA_ABASTECIMENTO_PA0, "PA0", false},  // Desativar Linha de Abastecimento
  {"FCV", PIN_VENT_PA1, "PA1", true},                  // Fechar Vent (Exceção: HIGH)
  {"ABV", PIN_VENT_PA1, "PA1", false},                 // Abrir Vent (Exceção: LOW)
  {"ATP", PIN_PURGA_LINHA_PA2, "PA2", true},           // Ativar Purga da Linha
  {"DTP", PIN_PURGA_LINHA_PA2, "PA2", false},          // Desativar Purga da Linha
  {"ATS", PIN_SUPERCHARGE_PA3, "PA3", true},           // Ativar Supercharge
  {"DTS", PIN_SUPERCHARGE_PA3, "PA3", false},          // Desativar Supercharge
  {"ATQ", PIN_QUICK_DISCONNECT_PA4, "PA4", true},       // Ativar Quick Disconnect
  {"DTQ", PIN_QUICK_DISCONNECT_PA4, "PA4", false},      // Desativar Quick Disconnect
  {"ATA", PIN_ARM_PA5, "PA5", true},                    // Ativar ARM
  {"DTA", PIN_ARM_PA5, "PA5", false},                   // Desativar ARM
  {"ATI", PIN_IGNICAO_PA6, "PA6", true},                // Ativar Ignição
  {"DTI", PIN_IGNICAO_PA6, "PA6", false},               // Desativar Ignição
  {"ATJ", PIN_INJECAO_PA7, "PA7", true},                // Ativar Injeção
  {"DTJ", PIN_INJECAO_PA7, "PA7", false},               // Desativar Injeção
  {"ATT", PIN_PURGA_TANQUE_PB0, "PB0", true},           // Ativar Purga do Tanque
  {"DTT", PIN_PURGA_TANQUE_PB0, "PB0", false},          // Desativar Purga do Tanque
};

// Número total de comandos
const uint8_t NUM_COMMANDS = sizeof(commandMap) / sizeof(Command);

// Tamanho do comando esperado
#define COMMAND_LENGTH 3

// Buffer para armazenar o comando recebido
char commandBuffer[COMMAND_LENGTH + 1]; // +1 para o terminador nulo
uint8_t commandIndex = 0;

void setup() {
  // Inicializa a porta serial USB para depuração a 9600 baud
  Serial.begin(9600);
  while (!Serial) {
    ; // Espera até que a porta serial esteja pronta
  }
  Serial.println("STM32 Recebedor de Comandos Iniciado.");

  // Inicializa a Serial1 para comunicação com o módulo E32 LoRa a 9600 baud
  Serial1.begin(9600);
  Serial.println("Serial1 (LoRa) iniciada a 9600 baud.");

  // Inicializa todos os pinos mapeados como OUTPUT e define como LOW inicialmente
  for (uint8_t i = 0; i < NUM_COMMANDS; i++) {
    pinMode(commandMap[i].pin, OUTPUT);
    digitalWrite(commandMap[i].pin, LOW);
  }

  Serial.println("Todos os pinos configurados como LOW.");
}

void loop() {
  // Verifica se há dados disponíveis na Serial1
  while (Serial1.available() > 0) {
    // Lê um byte da Serial1
    char incomingByte = Serial1.read();

    // Armazena o byte no buffer, se ainda houver espaço
    if (commandIndex < COMMAND_LENGTH) {
      commandBuffer[commandIndex++] = incomingByte;
    }

    // Quando o buffer estiver cheio, processa o comando
    if (commandIndex == COMMAND_LENGTH) {
      commandBuffer[COMMAND_LENGTH] = '\0'; // Adiciona o terminador nulo

      // Imprime o comando completo no terminal serial USB (para depuração)
      Serial.print("Comando recebido: ");
      Serial.println(commandBuffer);

      // Processa o comando recebido
      processCommand(commandBuffer);

      // Reseta o índice para o próximo comando
      commandIndex = 0;
    }
  }

  // Opcional: Pequeno atraso para estabilidade (pode ser ajustado ou removido)
  // delay(10);
}

// Função para processar e executar o comando recebido
void processCommand(char *cmd) {
  for (uint8_t i = 0; i < NUM_COMMANDS; i++) {
    if (strcmp(cmd, commandMap[i].code) == 0) {
      digitalWrite(commandMap[i].pin, commandMap[i].setHigh ? HIGH : LOW);
      Serial.print("Pino ");
      Serial.print(commandMap[i].pinName);
      Serial.print(" (");
      Serial.print(commandMap[i].code);
      Serial.print(") definido para ");
      Serial.println(commandMap[i].setHigh ? "HIGH" : "LOW");

      // Aguarda 1 segundo antes de enviar a confirmação
      delay(1000);

      // Tenta enviar a confirmação até 4 vezes, com 0.75 segundos de intervalo
      for (int tentativa = 1; tentativa <= 4; tentativa++) {
        Serial1.println(cmd); // Envia o mesmo comando como confirmação
        Serial.print("Tentativa de confirmação ");
        Serial.print(tentativa);
        Serial.println(" enviada.");

        // Aguarda 0.75 segundos antes da próxima tentativa, se houver
        if (tentativa < 4) {
          delay(750);
        }
      }

      return;
    }
  }

  // Se o comando não for encontrado no mapeamento
  Serial.print("Comando desconhecido: ");
  Serial.println(cmd);
}
