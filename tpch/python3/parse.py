from absl import app
from absl import flags
from absl import logging
import numpy as np
import os
import pdb
import pyarrow as pa
import pyarrow.csv as csv
import pyarrow.compute as pc
from schema import SCHEMA

SF = 0
table_pat = os.path.join(os.getenv("HOME"), "Downloads", "tpchv3_s1e%d" % SF,
                         "%s.tbl")
rawd_pat = os.path.join(os.getenv("HOME"), "Downloads", "tpchv3_s1e%d" % SF,
                        "%s", "%s.bin")


def main(argv):
    for tablename in SCHEMA:
        colnames = list(SCHEMA[tablename].keys())
        table = csv.read_csv(
            table_pat % tablename,
            read_options=csv.ReadOptions(column_names=colnames),
            parse_options=csv.ParseOptions(delimiter="|"))
        for i in range(len(colnames)):
            schema = SCHEMA[tablename][colnames[i]]
            if schema[0] == "string":
                continue
            fp = rawd_pat % (tablename, colnames[i])
            od = os.path.split(fp)[0]
            if (not os.path.exists(od)):
                logging.info("mkdir %s" % od)
                os.mkdir(od)
            logging.info("Writing %s" % fp)
            with open(fp, "wb") as fo:
                for chunk in table[i].iterchunks():
                    if schema[0] == "int32":
                        fo.write(chunk.to_numpy().astype(np.int32).tobytes())
                    elif schema[0] == "date":
                        fo.write(
                            chunk.cast(pa.int32()).to_numpy().astype(
                                np.int32).tobytes())
                    elif schema[0] == "decimal":
                        fo.write((chunk.to_numpy() * 100).astype(
                            np.int64).tobytes())
                    elif schema[0] == "int8":
                        npa = chunk.to_numpy(zero_copy_only=False)
                        ans = np.full((len(npa), ), len(schema[1]))
                        for k in range(len(schema[1])):
                            ans[npa == schema[1][k]] = k
                        assert (0 == np.count_nonzero(ans == len(schema[1])))
                        fo.write(ans.astype(np.int8).tobytes())
                    elif schema[0] == "int16":
                        npa = chunk.to_numpy(zero_copy_only=False)
                        ans = np.full((len(npa), ), len(schema[1]))
                        for k in range(len(schema[1])):
                            ans[npa == schema[1][k]] = k
                        assert (0 == np.count_nonzero(ans == len(schema[1])))
                        fo.write(ans.astype(np.int16).tobytes())


if __name__ == "__main__":
    app.run(main)
