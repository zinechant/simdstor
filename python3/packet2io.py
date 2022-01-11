from absl import app
from absl import flags
from absl import logging
from collections import deque

from trace_stream import trace_stream


def packet2io(pbpath, vague, reqnano, tsbsize, pagesize, offset):
    (ticknano, istream, ostream) = trace_stream(pbpath, vague)
    ticknano //= int(1e9)
    ns = len(istream) + len(ostream)
    pps = tsbsize // pagesize // ns
    logging.info("page per stream: %d", pps)

    iot = []

    for i in range(ns):
        s = istream[i] if i < len(istream) else ostream[i - len(istream)]

        c = {}
        d = {}
        lasttick = -1
        for addr, tick in s:
            assert tick > lasttick
            lasttick = tick
            tick //= ticknano
            pageaddr = (addr - offset) // pagesize
            if pageaddr not in c:
                c[pageaddr] = tick
            d[pageaddr] = tick

        p = deque()
        spp = pagesize >> 9

        if i < len(istream):
            for _ in range(pps):
                p.append(0)
            for addr in sorted(d.keys()):
                p.append(d[addr])
                x = p.popleft()
                y = c[addr]
                iot.append([x, y, addr * spp, spp, 1])
        else:
            for _ in range(pps):
                p.append(lasttick * 2 // ticknano)
            for addr in sorted(d.keys(), reverse=True):
                x = d[addr]
                y = p.popleft()
                p.append(c[addr])
                iot.append([x, y, addr * spp, spp, 0])

    iot = list(sorted(iot))
    for i in range(len(iot) - 1):
        iot[i + 1][0] = max(iot[i][0] + reqnano, iot[i + 1][0])

    return iot


def main(argv):
    F = flags.FLAGS
    iot = packet2io(F.pbpath, F.vague, F.reqnano, F.tsbsize, F.pagesize,
                    F.offset)
    for i in range(F.cores):
        with open(F.output % i, "w") as fo:
            for l in iot:
                l[2] += F.bias
                fo.write(" ".join(map(lambda x: str(x), l)))
                fo.write("\n")


if __name__ == "__main__":
    flags.DEFINE_string("pbpath", None, "path of the pb file")
    flags.DEFINE_integer("vague", 16, "vaugeness when compositing stream")
    flags.DEFINE_integer("reqnano", 10, "request latency in ns")
    flags.DEFINE_integer("tsbsize", 131072, "total streambuffer size")
    flags.DEFINE_integer("pagesize", 4096, "page size")
    flags.DEFINE_integer("offset", 1 << 40, "Address offset in packets")
    flags.DEFINE_integer("cores", 8, "number of cores")
    flags.DEFINE_integer("bias", (1 << 21) + 11, "Page address bias per core")
    flags.DEFINE_string("output", "streambuffer.io.%02d", "output re")

    flags.mark_flag_as_required("pbpath")
    app.run(main)
