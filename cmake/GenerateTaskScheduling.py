#!/usr/bin/env python3
import os, sys, datetime
from jinja2 import Template, Environment, FileSystemLoader
class TaskEntry:
    def __init__(self, name, rate):
        self.name = name
        self.rate = rate
        assert(type(self.name) is str)
        assert(type(self.rate) is int)

class Handler:
    def __init__(self, name, nvic_name, enable):
        self.name = name
        self.nvic_name = nvic_name
        self.default_enable = enable

if __name__ == "__main__":
    includes = []

    tasks = [
      TaskEntry("RunSerial", 1000),
    ]

    handlers = (
        Handler("USART0_IRQ", "USART0_IRQ", True),
    )

    template_directory = "/home/simon/software/LPC84x/generators/templates"
    env = Environment(loader=FileSystemLoader(template_directory))

    template_name = "main.cpp.j2"
    fname = template_name.strip(".j2")
    template = env.get_template(template_name)
    rendering = template.render(handlers=handlers, timestamp=datetime.datetime.now())
    with open(fname, 'w') as f:
        f.write(rendering)
