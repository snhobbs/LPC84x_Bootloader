import os, sys, datetime
from jinja2 import Template, Environment, FileSystemLoader

if __name__ == "__main__":
    template_dir = "/home/simon/software/LPC84x/generators"
    import sys
    sys.path.append(template_dir)
    from pins import *

    pins = [
        #Pin("reset", 0, 5, "pullup", SpecialtyPin(swm_name = "kSWM_RESETN")),
        #Pin("ispPin", 0, 12, "pullup", GPIO(is_input=True, default_value = False)),
        Pin("RxdUSART0", 0, 24, "pullup", SerialComm("usart", 0, "rxd")),
        Pin("TxdUSART0", 0, 25, "pullup", SerialComm("usart", 0, "txd")),
    ]

    env = Environment(loader=FileSystemLoader(template_dir + "/templates"))
    templates = ["IOPins.h.j2", "IOPins.cpp.j2", "Board.h.j2"]
    for tp in templates:
        fname = tp.strip(".j2")
        template = env.get_template(tp)
        rendering = template.render(pins=pins, timestamp=datetime.datetime.now())
        with open(fname, 'w') as f:
            f.write(rendering)

