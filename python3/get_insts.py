from absl import app
from absl import flags
from absl import logging
from parse_objdump import parse
import os
import re

DIR = os.path.split(os.path.split(__file__)[0])[0]
LZP = os.path.join(DIR, "lz", "src")
HUFP = os.path.join(DIR, "huffman", "src")

PROFILE = {
    "rvvc": {
        "file_path": os.path.join(LZP, "rvvc.c"),
        "ranges": {
            "Iter": [range(33, 34)],
            "Hash": [range(51, 63)],
            "Match": [range(64, 84)],
            "Grow": [range(85, 99)],
            "Delap": [range(102, 141)],
            "Encode": [range(142, 283)],
            "All": [range(1 << 30)],
        }
    },
    "rvvd": {
        "file_path": os.path.join(LZP, "rvvd.c"),
        "ranges": {
            "Iter": [range(53, 54)],
            "Unpack": [range(55, 65)],
            "Decode": [range(24, 34), range(77, 100)],
            "Copy": [range(121, 143)],
            "Loop": [range(113, 117), range(144, 150)],
            "All": [range(1 << 30)],
        }
    },
    "sepc": {
        "file_path": os.path.join(LZP, "sepc.c"),
        "ranges": {
            "All": [range(1 << 30)],
        }
    },
    "sepd": {
        "file_path": os.path.join(LZP, "sepd.c"),
        "ranges": {
            "All": [range(1 << 30)],
        }
    },
    "hdec": {
        "file_path": os.path.join(HUFP, "decode.cc"),
        "ranges": {
            "Iter": [range(38, 39)],
            "Setup": [range(38, 59)],
            "DP": [range(62, 88)],
            "Res": [range(89, 117)],
            "Over": [range(124, 144)],
            "All": [range(1 << 30)],
        }
    }
}


def get_insts(hist, reqs, line_dict):
    ans = {k: 0 for k in reqs["ranges"]}

    with open(hist, "r") as fi:
        line = fi.readline()
        hist_size = int(line.strip().split(":")[-1])
        for line in fi:
            hist_size -= 1
            l = line.split()
            ln, counts = int(l[0], 16), int(l[1])
            if ln in line_dict:
                for name in line_dict[ln]:
                    ans[name] += counts
        assert hist_size == 0, str(hist_size)

    return ans


def main(argv):
    FLAGS = flags.FLAGS
    ans = get_insts(FLAGS.hist, PROFILE[FLAGS.profile],
                    parse(FLAGS.objdump, PROFILE[FLAGS.profile]))
    for k in ans:
        print("%s\t%d" % (k, ans[k]))


if __name__ == "__main__":
    flags.DEFINE_string("objdump", None, "path of the objdump")
    flags.DEFINE_string("hist", None, "path of the inst hist")
    flags.DEFINE_enum("profile", None, PROFILE.keys(), " ")
    flags.mark_flag_as_required("objdump")
    flags.mark_flag_as_required("hist")
    flags.mark_flag_as_required("profile")
    app.run(main)
