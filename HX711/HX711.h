#ifndef HX711_H
#define HX711_H

#include "mbed.h"
#include <cstdint>

class HX711 {
private:
    PinName PD_SCK;
    PinName DOUT;
    DigitalOut *sck;
    DigitalIn *doutPin;

    uint8_t GAIN;
    long OFFSET = 0;
    float SCALE = 1;

    uint8_t shiftIn();

public:
    HX711();
    ~HX711();

    void begin(PinName dout, PinName pd_sck, uint8_t gain = 128);
    bool is_ready();
    void wait_ready(unsigned long delay_ms = 0);

    void set_gain(uint8_t gain = 128);
    long read();
    long read_average(uint8_t times = 10);
    double get_value(uint8_t times = 1);
    float get_units(uint8_t times = 1);

    void tare(uint8_t times = 10);
    void set_scale(float scale = 1.f);
    float get_scale();
    void set_offset(long offset = 0);
    long get_offset();

    void power_down();
    void power_up();
};

#endif
