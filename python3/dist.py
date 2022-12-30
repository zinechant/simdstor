import math
import numpy
import os
import pdb

MAXB = 64


def dist(words):
    stat = {i: 0 for i in range(MAXB)}
    for word in words:
        word = 0 - word if word < 0 else word

        b = 0 if word == 0 else math.ceil(math.log(word, 2))
        stat[b] += 1

    return stat


if __name__ == '__main__':
    DPATH = os.path.join(os.getenv("HOME"), "Downloads", "tpchv3_s1e0")
    FPATH = os.path.join(DPATH, "lineitem", "l_linenumber.bin")
    BS = 4

    words = []
    fsize = os.path.getsize(FPATH)
    with open(FPATH, "rb") as fi:
        for i in range(fsize // BS):
            words.append(
                int.from_bytes(fi.read(BS), byteorder='little', signed=True))

    stat = dist(words)
    tb = 0
    for i in range(MAXB):
        print("%10d," % i, end="")
        tb += stat[i]
    print()

    for i in range(MAXB):
        print("%10d," % stat[i], end="")
    print()

    NFILE = os.path.join(os.getenv("HOME"), "Downloads", "noisy_quant_GANs",
                         "main_0_p.npy")
    arr = numpy.load(NFILE).reshape([-1])
    stat2 = dist(arr)

    for i in range(MAXB):
        print("%10d," % stat2[i], end="")
    print()
