#include "HX711.h"
#include "mbed.h"
#include <cstdint>

// Construtor e destrutor
HX711::HX711() {}
HX711::~HX711() {}

// Inicialização dos pinos de dados e clock
void HX711::begin(PinName dout, PinName pd_sck, uint8_t gain) {
    this->PD_SCK = pd_sck;
    this->DOUT = dout;

    // Configuração dos pinos no Mbed
    sck =  new DigitalOut(PD_SCK);
    doutPin = new DigitalIn(DOUT);

    set_gain(gain);
}

bool HX711::is_ready() {
    return doutPin->read() == 0; // LOW indica pronto
}

void HX711::set_gain(uint8_t gain) {
    switch (gain) {
        case 128: GAIN = 1; break; // canal A, ganho 128
        case 64:  GAIN = 3; break; // canal A, ganho 64
        case 32:  GAIN = 2; break; // canal B, ganho 32
    }
}

long HX711::read() {
    // Aguarda até que o HX711 esteja pronto
    wait_ready();

    unsigned long value = 0;
    uint8_t data[3] = {0};
    uint8_t filler = 0x00;

    // Lê 24 bits de dados
    for (int i = 2; i >= 0; --i) {
        data[i] = shiftIn();
    }

    // Configura ganho para a próxima leitura
    for (int i = 0; i < GAIN; i++) {
        sck->write(1);
        wait_us(1);
        sck->write(0);
        wait_us(1);
    }

    // Sinaliza bit mais significativo para complementar
    if (data[2] & 0x80) {
        filler = 0xFF;
    }

    // Constrói o valor final
    value = (static_cast<unsigned long>(filler) << 24) |
            (static_cast<unsigned long>(data[2]) << 16) |
            (static_cast<unsigned long>(data[1]) << 8) |
            static_cast<unsigned long>(data[0]);

    return static_cast<long>(value);
}

uint8_t HX711::shiftIn() {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        sck->write(1);
        wait_us(1); // Pulso de clock
        value |= doutPin->read() << (7 - i);
        sck->write(0);
        wait_us(1);
    }
    return value;
}

void HX711::wait_ready(unsigned long delay_ms) {
    while (!is_ready()) {
        ThisThread::sleep_for(chrono::milliseconds(delay_ms));
    }
}

long HX711::read_average(uint8_t times) {
    long sum = 0;
    for (uint8_t i = 0; i < times; i++) {
        sum += read();
        ThisThread::sleep_for(chrono::milliseconds(0));
    }
    return sum / times;
}

double HX711::get_value(uint8_t times) {
    return read_average(times) - OFFSET;
}

float HX711::get_units(uint8_t times) {
    return get_value(times) / SCALE;
}

void HX711::tare(uint8_t times) {
    double sum = read_average(times);
    set_offset(sum);
}

void HX711::set_scale(float scale) {
    SCALE = scale;
}

float HX711::get_scale() {
    return SCALE;
}

void HX711::set_offset(long offset) {
    OFFSET = offset;
}

long HX711::get_offset() {
    return OFFSET;
}

void HX711::power_down() {
    sck->write(0);
    sck->write(1);
    ThisThread::sleep_for(chrono::milliseconds(60));
}

void HX711::power_up() {
    sck->write(0);
}
