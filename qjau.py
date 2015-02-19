#! /usr/bin/python

import argparse
import sys
import os
import subprocess

parser = argparse.ArgumentParser(prog='qjau', description='Set the socket prioirty and window size on an unmodified binary.')
parser.add_argument("-v", "--verbosity", type=int, help="Increase output verbosity")
parser.add_argument("-p", "--priority", type=int, help="Set the network socket prioirty [0-7]")
parser.add_argument("-w", "--window", type=int, help="Socket window size")
parser.add_argument("-c", "--command", help="Command that you wish to run", nargs="+")
args = parser.parse_args()

if args.priority == None:
    print "Error, priority argument required"
    parser.print_help()
    sys.exit(-1)

if args.verbosity == None:
    args.verbosity = 0


if args.command == None:
    print "Error, command argument required"
    parser.print_help()
    sys.exit(-1)

if args.window == None:
    print "Error, window size argument required"
    parser.print_help()
    sys.exit(-1)

if os.geteuid() != 0:
    print "Error, set_sock_priority must be run as root."
    sys.exit(-1)

new_env = os.environ
new_env["QJAU_VERBOSITY"] = str(args.verbosity)
new_env["QJAU_PRIORITY"]  = str(args.priority)
new_env["QJAU_WINDOW"]  = str(args.window)
new_env["LD_PRELOAD"]    = "./qjump-app-util.so"

cmd = " ".join(args.command)
try:
    subprocess.call(cmd,env=new_env, shell=True)
except:
    exit(0)

