from absl import app
from absl import flags
from absl import logging

import json
import os
import pdb

from schema import SCHEMA

BITS = {
    "decimal": 64,
    "date": 32,
    "int32": 32,
    "int16": 16,
    "int8": 8,
}

SBIT = {
    "decimal": 64,
    "date": 31,
    "int32": 32,
    "int16": 16,
    "int8": 8,
}

BWC = {
    64: "d",
    32: "w",
    16: "h",
    8: "b",
}

OPS = {
    "EqualTo": ("==", "svcmpeq", "svcmpeq", ("svvcmpeq", False)),
    "LessThan": ("<", "svcmplt", "svcmplt", ("svvcmplt", False)),
    "LessThanOrEqual": ("<=", "svcmple", "svcmple", ("svvcmple", False)),
    "GreaterThan": (">", "svcmpgt", "svcmpgt", ("svvcmplt", True)),
    "GreaterThanOrEqual": (">=", "svcmpge", "svcmpge", ("svvcmple", True)),
}


def f2cols(filter: list) -> set:
    if filter[0] == "Or":
        cols = set()
        for cf in filter[1]:
            cols |= f2cols(cf)
        return cols
    elif filter[0].startswith("c"):
        return set((filter[1], filter[2]))
    else:
        assert isinstance(filter[2], int)
        return set((filter[1], ))


def map2op(fstr: str, kind: int):
    scol = fstr.startswith('c')
    if scol:
        fstr = fstr[1:]
    return scol, OPS[fstr][kind]


def fcode0(filter: list, indent: int, pred: str, andor: str) -> str:
    scol, op = map2op(filter[0], 0)
    return "\n%s %s= (%s %s %s);" % (" " * indent + pred, andor,
                                     filter[1] + "_i[i]", op, filter[2] +
                                     "_i[i]" if scol else str(filter[2]))


def fcode1(filter: list, indent: int, pred: str, door: bool):
    scol, op = map2op(filter[0], 1)
    opstr = "%s(%s, v_%s, %s)" % (op, pred, filter[1],
                                  "v_" + filter[2] if scol else str(filter[2]))
    if door:
        return "\n%s = svorr_z(svptrue_b8(), %s, %s);" % (" " * indent + pred,
                                                          pred, opstr)
    else:
        return "\n%s = %s;" % (" " * indent + pred, opstr)


def fcode3(filter: list, indent: int, pred: str, door: bool, vin):
    scol, opb = map2op(filter[0], 3)
    op1 = "v_%s" % filter[1]
    op2 = "v_%s" % filter[2] if scol else str(filter[2])
    op, rev = opb
    ds = ""
    if not scol and (rev or filter[2] >= 16 or filter[2] < -16):
        op2 = "vi%d" % next(vin)
        ds = "\n%ssvint32_t %s = svvdup(%d);" % (" " * indent, op2, filter[2])
    if rev:
        op3 = op1
        op1 = op2
        op2 = op3
    opstr = "%s(%s, %s, %s)" % (op, pred, op1, op2)
    if door:
        return ds + "\n%s = svorr_z(svptrue_b8(), %s, %s);" % (
            " " * indent + pred, pred, opstr)
    else:
        return ds + "\n%s = %s;" % (" " * indent + pred, opstr)


def implgen(query, implpath, schema, tbdir, qname):
    filters = query["filters"]
    ocols = query["cols"]
    acols = set(ocols)

    for filter in filters:
        for col in f2cols(filter):
            assert col in schema, col
            acols.add(col)
    acols0 = next(iter(acols))

    BW = 32
    for col in acols:
        if schema[col][0] == "decimal":
            BW = 64
    ffp = os.path.join(tbdir, "%s.bin" % acols0)
    fs = os.path.getsize(ffp) << 3
    assert fs % BITS[schema[acols0][0]] == 0, ffp
    N = fs // BITS[schema[acols0][0]]

    code = """
#include <arm_sve.h>
#include <gem5/m5ops.h>

#include <cassert>
#include <string>

#include "debug.h"
#include "salloc.hh"

extern const std::string tdir;

void %s(int kind) {
  const int N = %d;
  unsigned e = 0;

""" % (qname, N)

    for col in acols:
        ty = "const int%d_t*" % BITS[schema[col][0]]
        code += '  %s %s_i = (%s)filedata((tdir + "%s/%s.bin").c_str());\n' % (
            ty, col, ty, qname.split('_')[-1], col)
    for col in ocols:
        ty = "int%d_t*" % BITS[schema[col][0]]
        code += '  %s %s_o = (%s)sballoc(N * %d);\n' % (
            ty, col, ty, BITS[schema[col][0]] >> 3)

    code += '  iprintf("'
    for col in acols:
        code += '%s_i=%%p, ' % col
    for col in ocols:
        code += '%s_o=%%p, ' % col
    code += '\\n"'
    for col in acols:
        code += ', %s_i' % col
    for col in ocols:
        code += ', %s_o' % col
    code += ');\n  m5_reset_stats(0, 0);\n'

    fc = ""
    sc = ""
    orn = 0
    for filter in filters:
        if filter[0] == "Or":
            pred = "keep%d" % orn
            orn += 1
            fc += "\n%sbool %s = false;" % (" " * 6, pred)
            for filter in filter[1]:
                fc += fcode0(filter, 6, pred, "|")
            fc += "\n%skeep &= %s;" % (" " * 6, pred)
        else:
            fc += fcode0(filter, 6, "keep", "&")
    for col in ocols:
        sc += "\n%s_o[e] = %s_i[i];" % (" " * 8 + col, col)
    sc += "\n%se++;" % (" " * 8)
    code += '''
  if (kind == 0) {
    for (int i = 0; i < N; i++) {
      bool keep = true;%s
      if (keep) {%s
      }
    }''' % (fc, sc)

    pc = ""
    fc = ""
    orn = 0
    for col in ocols:
        ty = "int%d_t*" % BITS[schema[col][0]]
        pc += '\n%s %s_p = %s_o;' % (" " * 4 + ty, col, col)
    svt = "svint%d_t" % BW
    fc += "\n%ssvbool_t pg = svwhilele_b%d_s32(i, N);" % (" " * 6, BW)
    for col in acols:
        bw = BITS[schema[col][0]]
        fc += "\n%s v_%s = svld1%s_s%d(pg, (const %s)%s_i);" % (
            " " * 6 + svt, col, "" if BW == bw else "u" + BWC[bw], BW,
            "int%d_t*" % bw if BW == bw else "uint%d_t*" % bw, col)
    fc += "\n"
    for filter in filters:
        if filter[0] == "Or":
            pred = "pg%d" % orn
            orn += 1
            fc += "\n%ssvbool_t %s = svpfalse();" % (" " * 6, pred)
            for filter in filter[1]:
                fc += fcode1(filter, 6, pred, True)
            fc += "\n%spg = svand_b_z(pg, pg, %s);" % (" " * 6, pred)
        else:
            fc += fcode1(filter, 6, "pg", False)
    fc += "\n\n%sunsigned d = svcntp_b%s(pg, pg);" % (" " * 6, BW)
    fc += "\n%se += d;\n%sassert(d <= (unsigned)n);" % (" " * 6, " " * 6)
    for col in ocols:
        bw = BITS[schema[col][0]]
        fc += ("\n%ssvst1%s_s%d(svwhilelt_b%d_u32(0, d), %s_p, "
               "svcompact(pg, v_%s));") % (" " * 6, "" if bw == BW else
                                           BWC[bw], BW, BW, col, col)
    fc += "\n"
    for col in acols:
        fc += "\n%s_i += n;" % (" " * 6 + col)
    for col in ocols:
        fc += "\n%s_p += d;" % (" " * 6 + col)
    code += '''
  } else if (kind == 1) {%s
    for (int i = 0, n = svcnt%s(); i < N; i += n) {%s
    }''' % (pc, "w" if BW == 32 else "d", fc)

    code += '''
  } else {'''
    for col in acols:
        bw = BITS[schema[col][0]]
        code += "\n%s_s = frstream((const int8_t*)%s_i, %d, N * %d);" % (
            " " * 4 + "unsigned " + col, col, bw, bw)
    for col in ocols:
        bw = BITS[schema[col][0]]
        code += "\n%s_t = fwstream((int8_t*)%s_o, %d);" % (
            " " * 4 + "unsigned " + col, col, bw)
    code += '\n%siprintf("' % (" " * 4)
    for col in acols:
        code += '%s_s=%%x, ' % col
    for col in ocols:
        code += '%s_t=%%x, ' % col
    code += '\\n"'
    for col in acols:
        code += ', %s_s' % col
    for col in ocols:
        code += ', %s_t' % col
    code += ');\n'

    fc = ""
    orn = 0
    svt = "svint%d_t" % BW
    fc += "\n%ssvbool_t pg = svwhilele_b%d_s32(i, N);" % (" " * 8, BW)
    for col in acols:
        bw = BITS[schema[col][0]]
        if bw == BW:
            fc += "\n%s v_%s = svunpack_s%d(%s_s);" % (" " * 8 + svt, col, BW,
                                                       col)
        else:
            fc += "\n%s v_%s = svld1%s_s%d(pg, (const %s)%s_i);" % (
                " " * 8 + svt, col, "u" + BWC[bw], BW, "uint%d_t*" % bw, col)
    fc += "\n"
    for filter in filters:
        if filter[0] == "Or":
            pred = "pg%d" % orn
            orn += 1
            fc += "\n%ssvbool_t %s = svpfalse();" % (" " * 8, pred)
            for filter in filter[1]:
                fc += fcode1(filter, 8, pred, True)
            fc += "\n%spg = svand_b_z(pg, pg, %s);" % (" " * 8, pred)
        else:
            fc += fcode1(filter, 8, "pg", False)
    for col in ocols:
        bw = BITS[schema[col][0]]
        fc += "\n%sunsigned e_%s = svpack_s%d(pg, v_%s, %s_t);" % (
            " " * 8, col, BW, col, col)
    fc += "\n%se += e_%s;" % (" " * 8, ocols[0])
    fc += "\n%sassert(e_%s <= (unsigned)n" % (" " * 8, ocols[0])
    for i in range(1, len(ocols)):
        fc += " && e_%s == e_%s" % (ocols[0], ocols[i])
    fc += ");\n"
    for col in acols:
        if BITS[schema[col][0]] != BW:
            fc += "\n%s_i += n;" % (" " * 8 + col)
    code += '''
    if (kind == 2) {
      for (int i = 0, n = svcnt%s(); i < N; i += n) {%s
      }''' % ("w" if BW == 32 else "d", fc)

    fc = ""
    orn = 0
    vin = iter(range(100))
    fcols = set()
    tfilters = []
    for filter in filters:
        bw = 8
        for col in f2cols(filter):
            bw = max(bw, SBIT[schema[col][0]])
        tfilters.append((bw, filter))
    for _, filter in sorted(tfilters):
        for col in f2cols(filter):
            if col not in fcols:
                fcols.add(col)
                fc += "\n%ssvint32_t v_%s = svvunpack(pg, %s_s);" % (" " * 8,
                                                                     col, col)
        if filter[0] == "Or":
            pred = "pg%d" % orn
            orn += 1
            fc += "\n%ssvbool_t %s = svpfalse();" % (" " * 8, pred)
            for filter in filter[1]:
                fc += fcode3(filter, 8, pred, True, vin)
            fc += "\n%spg = svand_b_z(pg, pg, %s);" % (" " * 8, pred)
        else:
            fc += fcode3(filter, 8, "pg", False, vin)
    for col in acols:
        if col not in fcols:
            fcols.add(col)
            fc += "\n%ssvint32_t v_%s = svvunpack(pg, %s_s);" % (" " * 8, col,
                                                                 col)
    fc += "\n"
    for col in ocols:
        fc += "\n%sunsigned e_%s = svvpack(pg, v_%s, %s_t);" % (" " * 8, col,
                                                                col, col)
    fc += "\n%sn = svvrcnum();\n%se += e_%s;" % (" " * 8, " " * 8, ocols[0])
    fc += "\n%sassert(e_%s <= (unsigned)n" % (" " * 8, ocols[0])
    for i in range(1, len(ocols)):
        fc += " && e_%s == e_%s" % (ocols[0], ocols[i])
    fc += ");\n"
    for col in acols:
        fc += "\n%scrstream(%s_s, n);" % (" " * 8, col)

    code += '''
    } else if (kind == 3) {
      for (int i = 0, n = 0; i < N; i += n) {
        svbool_t pg = svptrue_b8();%s
      }
    } else {
      fprintf(stderr, "%%s", "No this kind!\\n");
      return;
    }''' % (fc)

    for col in acols:
        code += "\n%sunsigned b_%s = erstream(%s_s);" % (" " * 4, col, col)
    for col in ocols:
        code += "\n%sunsigned c_%s = ewstream(%s_t);" % (" " * 4, col, col)

    code += '\n%siprintf("' % (" " * 4)
    for col in acols:
        code += 'b_%s=%%u, ' % col
    for col in ocols:
        code += 'c_%s=%%u, ' % col
    code += '\\n"'
    for col in acols:
        code += ', b_%s' % col
    for col in ocols:
        code += ', c_%s' % col
    code += ');'

    code += "\n%sassert(true" % (" " * 4)
    for col in ocols:
        code += " && c_%s == e * %d" % (col, BITS[schema[col][0]])
    code += ");"

    code += '''
  }
  m5_dump_stats(0, 0);

  iprintf("Total #elems: %u\\n", e);
'''
    for col in ocols:
        code += '\n  std::string %s_f("./%s_0.bin");' % (col, col)
    for col in ocols:
        code += '\n  %s_f[%s_f.length() - 5] += kind;' % (col, col)
    for col in ocols:
        code += '\n  filewrite(%s_f.c_str(), e * %d, (const int8_t*)%s_o);' % (
            col, BITS[schema[col][0]], col)
    code += "\n}\n"

    with open(implpath, 'w') as fo:
        fo.write(code)


def codegen(jsonpath, implpath, testpath, tdir):
    qnames = []
    with open(jsonpath, "r") as ji:
        querys = json.load(ji)
    for qid in querys:
        for tab in querys[qid]:
            query = querys[qid][tab]
            qname = "q%02d_%s" % (int(qid), tab)
            implgen(query, implpath % (int(qid), tab), SCHEMA[tab],
                    os.path.join(tdir, tab), qname)
            qnames.append(qname)

    externs = ""
    strcmps = ""
    for qname in qnames:
        externs += 'extern void %s(int kind);\n' % qname
        strcmps += ('  } else if (!strcmp(argv[1], "%s")) {\n'
                    '    %s(argv[2][0] - \'0\');\n') % (qname, qname)

    code = '''
#include <cstdio>
#include <cstring>
#include <string>

extern const std::string tdir = std::string("./data/");

extern void q02_part(int kind);
%s
int main(int argc, char* argv[]) {
  if (argc != 3 || argv[2][0] > '3' || argv[2][0] < '0') {
    fprintf(stderr, "Wrong Input! Usage: %%s <query> <0/1/2>\\n", argv[0]);
    return 1;
  }
  if (!strcmp(argv[1], "q02_part")) {
    q02_part(argv[2][0] - '0');
%s  } else {
    fprintf(stderr, "Wrong Query: %%s!\\n", argv[1]);
  }

  return 0;
}
''' % (externs, strcmps)

    with open(testpath, "w") as fo:
        fo.write(code)


def main(argv):
    FS = flags.FLAGS
    codegen(FS.jsonpath, FS.implpath, FS.testpath, FS.tdir)


if __name__ == "__main__":
    fdir = os.path.dirname(os.path.abspath(__file__))
    pdir = os.path.dirname(fdir)
    flags.DEFINE_string("jsonpath", os.path.join(fdir, "query.json"),
                        "jsonpath")
    flags.DEFINE_string("implpath", os.path.join(pdir, "src", "q%02d_%s.cc"),
                        "implpath")
    flags.DEFINE_string("testpath", os.path.join(pdir, "test", "test.cc"),
                        "testpath")
    flags.DEFINE_string("tdir", os.path.join(pdir, "test", "data"),
                        "tdir path")

    app.run(main)
