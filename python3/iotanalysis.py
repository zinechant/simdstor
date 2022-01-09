from absl import app
from absl import flags
from absl import logging


def iotanalysis(itp, otp):
    delays = 0
    with open(itp, "r") as fi, open(otp, "r") as fo:
        issues = []
        ddls = []
        latencys = []
        for line in fi:
            (issue, ddl, _, _, _) = map(lambda x: int(x), line.split())
            issues.append(issue)
            ddls.append(ddl)
        line = fo.readline()
        for line in fo:
            (_, _, latency) = map(lambda x: int(x), line.split())
            latencys.append(latency)

        assert len(ddls) == len(latencys)

        for i in range(len(issues)):
            delay = max(0, latencys[i] + issues[i] - ddls[i])
            if (delay):
                logging.info("%d %d %d %d %.3lf" %
                             (delay, issues[i], latencys[i], ddls[i],
                              float(delay) / latencys[i]))
                delays += delay

    return delays


def main(argv):
    F = flags.FLAGS
    print(iotanalysis(F.itp, F.otp))


if __name__ == "__main__":
    flags.DEFINE_string("itp", "streambuffer.io", "input trace path")
    flags.DEFINE_string("otp", "cstoragetrace.IO_Flow.No_0.log",
                        "output trace path")
    flags.DEFINE_integer("final", None, "final tick")
    app.run(main)
