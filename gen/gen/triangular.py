from .name_generator import NameGenerator as NG
import random

def triangular():
    metaroute_ix = {
        0: [0, 1],
        1: [1, 2],
        2: [0, 2]
    }

    ix = {n: NG.gen_stop_id() for n in range(3)}

    metaroute_fillers = {n: [] for n in range(9)}
    filler_size = 30
    for i in metaroute_fillers.keys():
        while len(metaroute_fillers[i]) < filler_size:
            candidate = NG.gen_stop_id()
            if candidate not in [ix[z] for z in metaroute_ix[i % 3]] \
                    and candidate not in metaroute_fillers[i]:
                metaroute_fillers[i] += candidate,

    metaroutes = {n: [*metaroute_fillers[3*n],
                      ix[metaroute_ix[n][0]],
                      *metaroute_fillers[3*n+1],
                      ix[metaroute_ix[n][1]],
                      *metaroute_fillers[3*n+2]] for n in range(3)}

    sectors = [*([2] * 5), *([3] * 14), *([4] * 10)]
    lines = {}
    mr_assoc = {n: {} for n in range(3)}

    for n in mr_assoc.keys():
        random.shuffle(sectors)
        line_ids = []
        while len(line_ids) < 5 + 14 + 10:
            candidate = NG.gen_line_id()
            if candidate not in lines:
                line_ids += candidate,
        i = 0
        for line, size in zip(line_ids, sectors):
            lines[line] = metaroutes[i:i+size]
            for z in metaroutes[i:i+size]:
                mr_assoc[n][z] = line
            i += size

