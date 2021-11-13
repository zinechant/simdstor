from absl import app
from absl import flags
from absl import logging
import os
import re

RES = [
    r"[0-9a-f]+\s+<(\w+)>:",
    r"(%s\S+):(\d+)" % os.getenv("HOME"),
    r"\s+([0-9a-f]+):\s+[0-9a-f]+\s+(.+)",
]


def parse(objdump, reqs):
    """returns a set of int for the counted code line"""
    with open(objdump, "r") as fi:
        rnames = None
        ans = {}
        for line in fi:
            m = re.match(RES[1], line)
            if m:
                file_path, line_number = m.groups()
                if os.path.samefile(reqs["file_path"], file_path):
                    line_number = int(line_number)
                    rnames = []
                    for n in reqs["ranges"]:
                        for r in reqs["ranges"][n]:
                            if line_number in r:
                                rnames.append(n)
                continue

            m = re.match(RES[0], line)
            if m:
                rnames = None
                continue

            if not rnames:
                continue

            m = re.match(RES[2], line)
            if m:
                lid, asm = m.groups()
                lid = int(lid, 16)
                ans[lid] = rnames

    return ans


def main(argv):
    FLAGS = flags.FLAGS
    print(parse(FLAGS.objdump, {"file_path": None, "ranges": []}))


if __name__ == "__main__":
    flags.DEFINE_string("objdump", None, "path of the objdump")
    flags.mark_flag_as_required("objdump")
    app.run(main)
