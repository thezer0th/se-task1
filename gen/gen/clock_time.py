import re


class ClockTime:
    def __init__(self):
        self.hr = None
        self.min = None

    @staticmethod
    def from_numbers(hour, minute):
        self = ClockTime()
        if 0 <= hour < 24 and 0 <= minute < 60:
            self.hr = hour
            self.min = minute
        else:
            raise ValueError("improper clock time")
        return self

    @staticmethod
    def from_ordinal(ordinal):
        self = ClockTime()
        if 0 <= ordinal < 24 * 60:
            self.hr, self.min = divmod(ordinal, 60)
        else:
            raise ValueError("improper ordinal")
        return self

    @staticmethod
    def from_str(clock_str):
        self = ClockTime()
        pat = r"(?P<hr>[1-9][0-9]*)\:(?P<min>[0-9]+)"
        m = re.match(pat, clock_str)
        if m:
            hr_num = int(m.group("hr"))
            min_num = int(m.group("min"))
            return self.from_numbers(hr_num, min_num)
        else:
            raise ValueError("improper clock str")

    def __str__(self):
        return f"{self.hr}:{self.min:02d}"

    def ord(self):
        return 60 * self.hr + self.min

    def __eq__(self, other):
        return self.ord() == other.ord()

    def __ne__(self, other):
        return self.ord() != other.ord()

    def __le__(self, other):
        return self.ord() <= other.ord()

    def __lt__(self, other):
        return self.ord() < other.ord()

    def __ge__(self, other):
        return self.ord() >= other.ord()

    def __gt__(self, other):
        return self.ord() > other.ord()
