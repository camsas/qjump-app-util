#! /usr/bin/python

# Copyright (c) 2015, Matthew P. Grosvenor
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the project, the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
    print ("Error, priority argument required")
    parser.print_help()
    sys.exit(-1)

if args.verbosity == None:
    args.verbosity = 0


if args.command == None:
    print ("Error, command argument required")
    parser.print_help()
    sys.exit(-1)

if args.window == None:
    print ("Error, window size argument required")
    parser.print_help()
    sys.exit(-1)

if os.geteuid() != 0:
    print ("Error, set_sock_priority must be run as root.")
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

