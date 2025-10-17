#!/usr/bin/env python

from array import array
from os import environ, getenv
from socket import socket, AF_UNIX, CMSG_LEN, SCM_RIGHTS, SOL_SOCKET
import struct
import sys
from typing import NamedTuple


class Request(NamedTuple):
    fds: array[int]
    argv: list[str]
    env: dict[str]


class Writer:
    buffs: list[bytes]

    def __init__(self):
        self.buffs = []

    def write_int(self, val: int):
        self.buffs.append(struct.pack("i", val))

    def write_str(self, val: str, encoding="utf-8"):
        self.write_int(len(val))
        self.buffs.append(val.encode(encoding))

    def write_str_list(self, val: list[str], encoding="utf-8"):
        self.write_int(len(val))
        for item in val:
            self.write_str(item, encoding)

    def write_str_dict(self, val: dict[str, str], encoding="utf-8"):
        self.write_int(len(val))
        for key in val:
            self.write_str(key, encoding)
            self.write_str(val[key], encoding)


def send_request(sock: socket, req: Request):
    msg2 = Writer()
    msg2.write_str_list(req.argv)
    msg2.write_str_dict(req.env)
    msg2 = msg2.buffs

    version = 1
    msg2_len = sum(len(b) for b in msg2)
    msg = struct.pack("ii", version, msg2_len)
    ancdata = (SOL_SOCKET, SCM_RIGHTS, req.fds)
    sock.sendmsg([msg], [ancdata])

    sock.sendmsg(msg2)


def await_response(sock: socket) -> int:
    sock.setblocking(True)
    msg = sock.recv(4)
    return struct.unpack("i", msg)[0]


def main() -> int:
    sock_path = getenv("PYSESSION_SOCKET") or "./.py.sock"

    sock = socket(AF_UNIX)
    sock.connect(sock_path)

    fds = array("i", (0, 1, 2))
    argv = sys.argv
    env = dict(environ)

    req = Request(fds, argv, env)
    send_request(sock, req)

    code = await_response(sock)
    return code


if __name__ == "__main__":
    exit(main())
