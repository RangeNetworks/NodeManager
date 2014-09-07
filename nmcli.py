#!/usr/bin/env python
#
# Copyright 2014 Range Networks, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# See the COPYING file in the main directory for details.
#

import sys
import zmq

addresses = {
	"openbts" : "tcp://127.0.0.1:45060",
	"smqueue" : "tcp://127.0.0.1:45063",
	"sipauthserve" : "tcp://127.0.0.1:45064"
}

def usage(ret=0):
	print "targets:"
	print "  openbts"
	print "  smqueue"
	print "  sipauthserve"
	print "command examples:"
	print "  ./nmcli.py target version                                      (query the component version)"
	print "  ./nmcli.py target config read                                  (show all configuration values)"
	print "  ./nmcli.py target config read Log.Level                        (show a specific configuration value)"
	print "  ./nmcli.py target config update Log.Level INFO                 (update a specific configuration value)"
	print "  ./nmcli.py openbts monitor                                     (query some live radio data)"
	print "  ./nmcli.py sipauthserve subscribers read                       (show all subscribers)"
	print "  ./nmcli.py sipauthserve subscribers create name imsi msisdn    (create a subscriber which uses cache auth)"
	print "  ./nmcli.py sipauthserve subscribers create name imsi msisdn ki (create a subscriber which uses full auth)"
	print "  ./nmcli.py sipauthserve subscribers delete imsi the-imsi       (delete a subscriber by imsi)"
	print "  ./nmcli.py sipauthserve subscribers delete msisdn the-msisdn   (delete a subscriber by imsi)"
	print "response codes:"
	print "  200 : action ok with response data"
	print "  204 : action ok with no response data"
	print "  204 : action ok with no change"
	print "  404 : unknown key or action"
	print "  406 : request is invalid"
	print "  409 : conflicting value"
	print "  500 : storing new value failed"
	print "usage:"
	print "  ./nmcgi.py target command (action (key (value)))"
	sys.exit(ret)

if len(sys.argv) < 3:
	usage(1)
target = sys.argv[1]
command = sys.argv[2]
action = ""
key = ""
value = ""
if len(sys.argv) > 3:
	action = sys.argv[3]
if len(sys.argv) > 4:
	key = sys.argv[4]
if len(sys.argv) > 5:
	value = sys.argv[5]

# some ugly one-off processing for manipulating subscribers, too useful to leave out
if target == "sipauthserve" and command == "subscribers" and action == "create":
	if len(sys.argv) != 7 and len(sys.argv) != 8:
		usage(1)
	name = key
	imsi = value
	msisdn = sys.argv[6]
	if len(sys.argv) == 8:
		ki = sys.argv[7]
	else:
		ki = ""

if target == "sipauthserve" and command == "subscribers" and action == "delete":
	if len(sys.argv) != 6:
		usage(1)
	fieldName = key
	fieldValue = value

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect(addresses[target])

request = '{"command":"' + command + '","action":"' + action + '","key":"' + key + '","value":"' + value + '"}'
if target == "sipauthserve" and command == "subscribers" and action == "create":
	request = '{"command":"subscribers","action":"create","fields":{"name":"' + name + '","imsi":"' + imsi + '","msisdn":"' + msisdn + '","ki":"' + ki + '"}}'
if target == "sipauthserve" and command == "subscribers" and action == "delete":
	request = '{"command":"subscribers","action":"delete","match":{"' + fieldName + '":"' + fieldValue + '"}}'
print "raw request: " + request
socket.send(request)

response = socket.recv()
print "raw response: " + response
