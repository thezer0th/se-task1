import random
import string
from .clock_time import ClockTime


class NameGenerator:
    line_id_chars = list(map(str, range(10)))
    stop_id_chars = string.ascii_letters + "_^"
    ticket_id_chars = string.ascii_letters + " "

    def __init__(self):
        pass

    @staticmethod
    def gen_line_id(id_len=8):
        return "".join(random.choices(NameGenerator.line_id_chars, k=id_len))

    @staticmethod
    def gen_time(lower, upper):
        """takes random time from [lower; upper)"""
        gen_time_ord = random.randrange(lower.ord(), upper.ord())
        return ClockTime.from_ordinal(gen_time_ord)

    @staticmethod
    def gen_stop_id(id_len=16):
        return "".join(random.choices(NameGenerator.stop_id_chars, k=id_len))

    @staticmethod
    def gen_ticket_id(id_len=16):
        return "".join(random.choices(NameGenerator.ticket_id_chars, k=id_len))

