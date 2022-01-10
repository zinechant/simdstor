from absl import app
from absl import flags
from absl import logging

import gzip
import packet_pb2
import struct


def _DecodeVarint32(in_file):
    """
    The decoding of the Varint32 is copied from
    google.protobuf.internal.decoder and is only repeated here to
    avoid depending on the internal functions in the library. If the
    end of file is reached, return (0, 0).
    """
    result = 0
    shift = 0
    pos = 0
    # Use a 32-bit mask
    mask = 0xffffffff
    while 1:
        c = in_file.read(1)
        if len(c) == 0:
            return (0, 0)
        b = struct.unpack('<B', c)[0]
        result |= ((b & 0x7f) << shift)
        pos += 1
        if not (b & 0x80):
            if result > 0x7fffffffffffffff:
                result -= (1 << 64)
                result |= ~mask
            else:
                result &= mask
            return (result, pos)
        shift += 7
        if shift >= 64:
            raise IOError('Too many bytes when decoding varint.')


def opengzip(in_file):
    """
    This opens the file passed as argument for reading using an appropriate
    function depending on if it is gzipped or not. It returns the file
    handle.
    """
    try:
        # First see if this file is gzipped
        try:
            # Opening the file works even if it is not a gzip file
            proto_in = gzip.open(in_file, 'rb')

            # Force a check of the magic number by seeking in the
            # file. If we do not do it here the error will occur when
            # reading the first message.
            proto_in.seek(1)
            proto_in.seek(0)
        except IOError:
            proto_in = open(in_file, 'rb')
    except IOError:
        print("Failed to open ", in_file, " for reading")
        exit(-1)
    return proto_in


def decodeMessage(in_file, message):
    """
    Attempt to read a message from the file and decode it. Return
    False if no message could be read.
    """
    try:
        size, pos = _DecodeVarint32(in_file)
        if size == 0:
            return False
        buf = in_file.read(size)
        message.ParseFromString(buf)
        return True
    except IOError:
        return False


def trace_stream(pbpath, vague):
    istream = {}
    ostream = {}

    with opengzip(pbpath) as pi:
        assert pi.read(4).decode() == "gem5"

        header = packet_pb2.PacketHeader()
        assert decodeMessage(pi, header) == True

        logging.info("Object id: %s" % header.obj_id)
        logging.info("Tick frequency: %s" % header.tick_freq)
        for id_string in header.id_strings:
            logging.info('Master id %d: %s' % (id_string.key, id_string.value))

        num_packets = 0
        packet = packet_pb2.Packet()

        while decodeMessage(pi, packet):
            num_packets += 1
            if packet.cmd != 1 and packet.cmd != 4:
                logging.warning(
                    "Ignore unknown cmd=%d, addr=%s, size=%s, tick=%s" %
                    (packet.cmd, packet.addr, packet.size, packet.tick))
                continue
            stream = istream if packet.cmd == 1 else ostream

            oaddr = int(packet.addr)
            naddr = oaddr + int(packet.size)

            for v in range(vague):
                vaddr = oaddr - v
                if vaddr in stream:
                    stream[naddr] = stream[vaddr]
                    del stream[vaddr]
                    stream[naddr].append((oaddr, int(packet.tick)))
                    break
                vaddr = None
            if vaddr is None:
                stream[naddr] = [(oaddr, int(packet.tick))]

    logging.info("packets: %d, istream: %d, ostream: %d" %
                 (num_packets, len(istream), len(ostream)))

    return (int(header.tick_freq), list(istream.values()),
            list(ostream.values()))


def main(argv):
    FLAGS = flags.FLAGS
    ans = trace_stream(FLAGS.pbpath, FLAGS.vague)


if __name__ == "__main__":
    flags.DEFINE_string("pbpath", None, "path of the pb file")
    flags.DEFINE_integer("vague", 1, "vaugeness when compositing stream")
    flags.mark_flag_as_required("pbpath")
    app.run(main)
