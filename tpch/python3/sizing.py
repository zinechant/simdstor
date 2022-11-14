import json
import math
import os

from schema import SCHEMA
from codegen import BITS


def sizing(fpath, bits):
    osize = 0
    nsize = 0
    fsize = os.path.getsize(fpath)
    bs = bits >> 3
    with open(fpath, "rb") as fi:
        for i in range(fsize // bs):
            word = int.from_bytes(fi.read(bs), byteorder='little', signed=True)
            word = 0 - word if word < 0 else word

            osize += bits
            nsize += 0 if word == 0 else math.ceil(math.log(word, 2))
            nsize += 6

    return osize, nsize


def stat(dpath):
    stat = {}
    for table in SCHEMA:
        stat[table] = {}
        for column in SCHEMA[table]:
            if SCHEMA[table][column][0] == 'string':
                continue
            osize, nsize = sizing(os.path.join(dpath, table, column + ".bin"),
                                  BITS[SCHEMA[table][column][0]])
            stat[table][column] = (osize, nsize)
    return stat


def sizes(dpath, qpath, spath):
    if not os.path.exists(spath):
        stats = stat(dpath)
        with open(spath, "w") as jo:
            json.dump(stats, jo)
    else:
        with open(spath, "r") as ji:
            stats = json.load(ji)

    with open(qpath, "r") as ji:
        query = json.load(ji)

    for i in sorted([int(i) for i in query.keys()]):
        for table in query[str(i)]:
            cols = query[str(i)][table]["cols"]
            osize = 0
            nsize = 0
            for col in cols:
                osize += stats[table][col][0]
                nsize += stats[table][col][1]
            print("q%02d%s\t%d\t%d" % (i, table[0], osize, nsize))


if __name__ == '__main__':
    DPATH = os.path.join(os.getenv("HOME"), "Downloads", "tpchv3_s1e0")
    QPATH = "query.json"
    SPATH = "sizes.json"
    sizes(DPATH, QPATH, SPATH)

    for i in ("cb", "cw", "db", "dw"):
        print(
            i,
            str(
                sizing(
                    os.path.join(os.getenv("HOME"), "Downloads", "nncd",
                                 "prune", "%s.data" % i), 32)))
