from absl import app
from absl import flags
from absl import logging
from collections import deque


def iotanalysis(itp, otp):
    delays = 0
    with open(itp, "r") as fi, open(otp, "r") as fo:
        issues = []
        ddls = []
        latencys = []
        line = fo.readline()
        for line in fi:
            (issue, ddl, addr, _, _) = map(lambda x: int(x), line.split())
            line = fo.readline()
            (_, latency, _) = map(lambda x: int(x), line.split())
            issues.append(issue)
            ddls.append(ddl)
            latencys.append(latency)

        queue = deque()
        d = 0
        for i in range(len(issues)):
            nano = issues[i] + delays
            while len(queue) and queue[0][1] <= nano:
                if queue[0][0] + queue[0][2] > nano:
                    d += 1
                    delay = queue[0][0] + queue[0][2] - nano
                    delays += delay
                    logging.debug("d=%d issue=%d slack=%d lat=%d d/s=%.3lf" %
                                  (delay, queue[0][0], queue[0][1] -
                                   queue[0][0], queue[0][2], float(delay) /
                                   (queue[0][1] - queue[0][0])))
                queue.popleft()
                nano = issues[i] + delays

            queue.append((issues[i] + delays, ddls[i] + delays, latencys[i]))

    logging.info("%d analyzed, %d delayed" % (len(issues), d))

    return delays


def main(argv):
    F = flags.FLAGS
    print(iotanalysis(F.itp, F.otp))


if __name__ == "__main__":
    flags.DEFINE_string("itp", "streambuffer.io.00", "input trace path")
    flags.DEFINE_string("otp", "cstoragetrace.IO_Flow.No_0.log",
                        "output trace path")
    app.run(main)
