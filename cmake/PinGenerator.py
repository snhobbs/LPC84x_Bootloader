import os, sys, datetime
from jinja2 import Template, Environment, FileSystemLoader

if __name__ == "__main__":
    template_dir = "/home/simon/software/LPC84x/generators"
    import sys
    sys.path.append(template_dir)
    from pins import *

#ifdef USE_RS485_RTS
#extern const MovablePin usart0_rts{Pins::PIO0_00, IOCONMode::pullup, kSWM_USART0_RTS};
#else
#GPIOPin usart0_rts{Pins::PIO0_00, IOCONMode::repeater, kGPIO_DigitalOutput, false};
#endif

    pins = [
        Pin("usart0_rts", 0, 0, "pullup", SerialComm("usart", 0, "rts")),
        Pin("ntc", 0, 1, "pullup", GPIO(is_input=True)),
        Pin("tms_swdio_swd", 0, 2, "inact", SpecialtyPin(swm_name = "kSWM_SWDIO")),
        Pin("tck_swclk_swd", 0, 3, "inact", SpecialtyPin(swm_name = "kSWM_SWCLK")),
        Pin("p1", 0, 4, "repeater", GPIO(is_input=False, default_value = False)),
        Pin("reset", 0, 5, "pullup", SpecialtyPin(swm_name = "kSWM_RESETN")),
        Pin("vlo_adc", 0, 6, "adc", ADC(1)),
        Pin("pwm1", 0, 7, "pwm", MatchRegister(1)),
        Pin("cclk", 0, 8, "pwm", ClockOut()),
        Pin("load", 0, 9, "repeater", GPIO(is_input=False, default_value = False)),
        Pin("scl", 0, 10, "I2C0_standard", I2C0(function="scl")),
        Pin("sda", 0, 11, "I2C0_standard", I2C0(function="sda")),
        Pin("ispPin", 0, 12, "pullup", GPIO(is_input=True, default_value = False)),
        Pin("p0", 0, 13, "repeater", GPIO(is_input=False, default_value = False)),
        Pin("nalm", 0, 14, "repeater", GPIO(is_input=True, default_value = True)),
        Pin("ncs_adc", 0, 15, "pullup", SerialComm("spi", 0, "ssel0")),
        Pin("sclk_0", 0, 16, "pullup", SerialComm("spi", 0, "sck")),
        Pin("thold", 0, 17, "dac", DAC(0)),
        Pin("vtherm_adc", 0, 18, "adc", ADC(8)),
        Pin("p23test_adc", 0, 19, "adc", ADC(7)),
        Pin("pwm0", 0, 20, "pwm", MatchRegister(0)),
        Pin("p5atest_adc", 0, 21, "adc", ADC(5)),
        Pin("led_red", 0, 22, "inact_OD", GPIO(is_input=False, default_value = True)),
        Pin("led_blue", 0, 23, "inact_OD", GPIO(is_input=False, default_value = True)),
        Pin("RxdUSART0", 0, 24, "pullup", SerialComm("usart", 0, "rxd")),
        Pin("TxdUSART0", 0, 25, "pullup", SerialComm("usart", 0, "txd")),
        Pin("miso_0", 0, 26, "pullup", SerialComm("spi", 0, "miso")),
        Pin("tp2", 0, 27, "repeater", GPIO(is_input=False, default_value = False)),
        Pin("p2", 0, 28, "repeater", GPIO(is_input=False, default_value = False)),
    ]

    env = Environment(loader=FileSystemLoader(template_dir + "/templates"))
    templates = ["IOPins.h.j2", "IOPins.cpp.j2", "Board.h.j2"]
    for tp in templates:
        fname = tp.strip(".j2")
        template = env.get_template(tp)
        rendering = template.render(pins=pins, timestamp=datetime.datetime.now())
        with open(fname, 'w') as f:
            f.write(rendering)

