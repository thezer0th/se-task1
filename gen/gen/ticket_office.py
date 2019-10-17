import re
import itertools as it
from .clock_time import ClockTime
import sys

class TicketOffice:
    def __init__(self):
        self.lines = {}
        self.tickets = set(), {}
        self.counter = 0
        self.i = 1

    def process_line_add(self, request: str):
        [line_id, *tail] = request.split(" ")
        times, stops = list(map(ClockTime.from_str, tail[0::2])), tail[1::2]

        line = {}
        for i, (time, stop) in enumerate(zip(times, stops)):
            if stop in line:
                raise ValueError("such a stop already in line")
            else:
                line[stop] = (i, time)

        if line_id in self.lines:
            raise ValueError("such a line already present")

        self.lines[line_id] = line
        return None

    def process_ticket_add(self, match):
        [ticket_id, price, dur] = list(map(match.group, ["id", "price", "dur"]))
        price = float(price)
        dur = int(dur)
        ids, tickets = self.tickets
        if ticket_id in ids:
            raise ValueError("ticket already present")

        tickets[ticket_id] = price, dur
        return None

    def verify_seq(self, seq):
        prev_time = None
        for start, line, end in list(zip(seq, seq[1:], seq[2:]))[::2]:
            if start not in self.lines[line] or end not in self.lines[line]:
                raise ValueError("endpoints not in line")
            start_i, start_time = self.lines[line][start]
            end_i, end_time = self.lines[line][end]
            if start_i >= end_i:
                raise ValueError("improper seq indices")
            if prev_time and prev_time > start_time:
                raise ValueError("non-causal scenario")
            if prev_time and prev_time < start_time:
                return start
            prev_time = end_time
        return None

    def find_ticket_set(self, dur):
        _, price_map = self.tickets
        ids = price_map.keys()
        all_options = it.chain(it.product(ids, repeat=1), it.product(ids, repeat=2), it.product(ids, repeat=3))
        best_price = None
        best_set = None
        for id_set in all_options:
            total_price = sum(map(lambda pd: price_map[pd][0], id_set))
            total_dur = sum(map(lambda pd: price_map[pd][1], id_set))
            if total_dur >= dur and (not best_price or best_price > total_price):
                best_price = total_price
                best_set = id_set
        return best_set

    def process_route_query(self, request: str):
        seq = list(request.split(" "))[1:]
        wait_stop = self.verify_seq(seq)
        if wait_stop:
            return f":-( {wait_stop}"

        _, start = self.lines[seq[1]][seq[0]]
        _, end = self.lines[seq[-2]][seq[-1]]
        dur = end.ord() - start.ord()

        tickets = self.find_ticket_set(dur + 1)
        if tickets:
            self.counter += len(tickets)
            return "! " + "".join(map(lambda s: f" {s};", tickets))
        else:
            return ":-|"

    line_add_re = r"^[0-9]+(\ [1-9][0-9]*:[0-9][0-9]\ [a-zA-Z_\^]+)+$"
    ticket_add_re = r"^(?P<id>[a-zA-Z\ ]+?)\ (?P<price>[0-9]*\.[0-9][0-9])\ (?P<dur>[1-9][0-9]*)$"
    ticket_query_re = r"^\?\ [a-zA-Z_\^]+(\ [0-9]+\ [a-zA-Z_\^]+)+$"

    def process(self, request):
        try:
            m = re.match(self.line_add_re, request)
            if m:
                return True, self.process_line_add(request)

            m = re.match(self.ticket_add_re, request)
            if m:
                return True, self.process_ticket_add(m)

            m = re.match(self.ticket_query_re, request)
            if m:
                return True, self.process_route_query(request)

            if request == "":
                return True, None
            else:
                raise ValueError("unrecognized format")
        except ValueError:
            return False, None

    def run(self, req):
        status, out = self.process(req)
        err = None if status else f"Error in line {self.i}: {req}"
        if out:
            sys.stdout.write(f"{out}\n")
        if err:
            sys.stderr.write(f"{err}\n")
        self.i += 1

