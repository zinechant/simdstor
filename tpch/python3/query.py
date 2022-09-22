from absl import app
from absl import flags
from absl import logging
from datetime import datetime

import json
import os
import pdb
import re

from schema import SCHEMA

RES = (
    r"Q1(\d+)\.err\.txt:\d+",
    r"Q1\d+\.err\.txt-Pushing operators to (\w+)@",
    r"Q1\d+\.err\.txt-Pushed Filters: (.*)$",
    r"Q1\d+\.err\.txt-Post-Scan Filters: (.*)$",
    r"Q1\d+\.err\.txt-Output: (.*)$",
)

OP = {
    ">": "GreaterThan",
    ">=": "GreaterThanOrEqual",
    "<": "LessThan",
    "<=": "LessThanOrEqual",
    "==": "EqualTo"
}

EPOCHDAY = datetime.fromisoformat('1970-01-01')


def illegal(schema, col):
    if schema[col][0] == "string":
        return True
    return False


def filter(fstr, schema):
    x = fstr.find("(")
    y = len(fstr) - 1
    assert fstr[y] == ")", fstr
    op = fstr[0:x]
    if op == "Or":
        z = 0
        l = 0
        for i in range(x + 1, y):
            if fstr[i] == "(":
                l += 1
            elif fstr[i] == ")":
                l -= 1
            elif fstr[i] == ",":
                if (l == 0):
                    z = i
                    break
        assert z != 0, fstr
        fs = []
        l = filter(fstr[x + 1:z], schema)
        r = filter(fstr[z + 1:y], schema)
        if l is None:
            return None
        if r is None:
            return None
        if l[0] == "Or":
            fs += l[1]
        else:
            fs.append(l)
        if r[0] == "Or":
            fs += r[1]
        else:
            fs.append(r)
        return (op, fs)
    else:
        assert op in list(OP.values()), fstr
        z = fstr.find(",")
        col = fstr[x + 1:z]
        vstr = fstr[z + 1:y]
        if illegal(schema, col):
            return None
        if schema[col][0] == "decimal":
            val = int(float(vstr) * 100)
        elif schema[col][0] == "date":
            val = (datetime.fromisoformat(vstr) - EPOCHDAY).days
        elif schema[col][0] == "int32":
            val = int(vstr)
        else:
            assert schema[col][0].startswith("int"), schema[col]
            if vstr not in schema[col][1]:
                pdb.set_trace()
            val = schema[col][1].index(vstr)
        return (op, col, val)


def pushed(match, schema):
    if match is None:
        return []
    filters = []
    for fstr in match[1].split(", "):
        f = filter(fstr, schema)
        if f is None:
            return None
        filters.append(f)
    return filters


def postscan(match, schema):
    if match is None:
        return []
    filters = []
    for fstr in match[1].split(","):
        assert fstr[0] == "(", fstr
        assert fstr[-1] == ")", fstr
        fstr = fstr[1:-1]
        x = fstr.find(" ")
        y = fstr.rfind(" ")
        op = "c" + OP[fstr[x + 1:y]]
        coll = re.sub("#\d+$", "", fstr[:x])
        colr = re.sub("#\d+$", "", fstr[y + 1:])
        if illegal(schema, coll):
            return None
        if illegal(schema, colr):
            return None
        filters.append((op, coll, colr))
    return filters


def columns(match, schema):
    cols = match[1].split(", ")
    for i in range(len(cols)):
        cols[i] = re.sub("#\d+$", "", cols[i])
        if illegal(schema, cols[i]):
            return None
    return cols


def res(schema, ls):
    filters = pushed(re.match(RES[2], ls[2]), schema)
    if filters is None:
        return None

    tmpfs = postscan(re.match(RES[3], ls[3]), schema)
    if tmpfs is None:
        return None
    filters += tmpfs

    cols = columns(re.match(RES[4], ls[4]), schema)
    if cols is None:
        return None

    return {"filters": filters, "cols": cols}


def query(logpath, jsonpath):
    ans = {}
    ls = [None] * 5
    with open(logpath, "r") as fi:
        for l in fi:
            ls[0] = l.strip()
            for i in range(1, 5):
                ls[i] = fi.readline().strip()
                assert (ls[i][:12] == ls[0][:12])

            la = " | ".join(ls)
            if ls[2][-1] == ":" and ls[3][-1] == ":":
                logging.info("Ignoring EmptyFs\t %s" % la)
                continue
            if "String" in la or "UDF" in la:
                logging.info("Ignoring StringUDF\t %s" % la)
                continue

            qid = int(re.match(RES[0], ls[0])[1]) - 20
            tab = re.match(RES[1], ls[1])[1]
            schema = SCHEMA[tab]
            query = res(schema, ls)
            if query is None:
                logging.info("Ignoring StringCol\t %s" % la)
            else:
                if qid not in ans:
                    ans[qid] = {}
                if tab in ans[qid]:
                    if query != ans[qid][tab]:
                        logging.error("Unequal filters: %s -- %s" %
                                      (str(ans[qid][tab]), str(query)))
                else:
                    ans[qid][tab] = query

    with open(jsonpath, "w") as jo:
        json.dump(ans, jo, indent=4)


def main(argv):
    query(flags.FLAGS.logpath, flags.FLAGS.jsonpath)


if __name__ == "__main__":
    flags.DEFINE_string("logpath",
                        os.path.join(os.path.split(__file__)[0], "spark.err"),
                        "logpath")
    flags.DEFINE_string("jsonpath",
                        os.path.join(os.path.split(__file__)[0], "query.json"),
                        "jsonpath")
    app.run(main)
