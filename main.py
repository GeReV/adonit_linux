#!/usr/bin/env python
# Michael Saunby. April 2013
#
# Notes.
# pexpect uses regular expression so characters that have special meaning
# in regular expressions, e.g. [ and ] must be escaped with a backslash.
#
#   Copyright 2013 Michael Saunby
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
import pexpect
import sys


def floatfromhex(h):
    t = float.fromhex(h)
    if t > float.fromhex('7FFF'):
        t = -(float.fromhex('FFFF') - t)
        pass
    return t


class Device:

    def __init__(self, bluetooth_adr):
        self.con = pexpect.spawn('gatttool -b ' + bluetooth_adr +
                                 ' --interactive')
        self.con.expect('\[LE\]>', timeout=600)
        print "Connecting..."
        self.con.sendline('connect')
        # test for success of connect
        # self.con.expect('Connection successful.*\[LE\]>')
        self.con.expect('\[CON\].*>')
        self.cb = {}
        print "Connected."
        return

    def char_write_cmd(self, handle, value):
        # The 0%x for value is VERY naughty!  Fix this!
        cmd = 'char-write-cmd 0x%02x 0%x' % (handle, value)
        print cmd
        self.con.sendline(cmd)
        return

    def char_read_uuid(self, handle):
        self.con.sendline('char-read-uuid %s' % handle)
        self.con.expect('handle: .*? \r')
        after = self.con.after
        rval = after.split()[1:1] + after.split()[:2]
        return [long(float.fromhex(n)) for n in rval]


    def char_read_hnd(self, handle):
        self.con.sendline('char-read-hnd 0x%02x' % handle)
        self.con.expect('descriptor: .*? \r')
        after = self.con.after
        rval = after.split()[1:]
        return [long(float.fromhex(n)) for n in rval]

    # Notification handle = 0x0025 value: 9b ff 54 07
    def notification_loop(self):
        while True:
            try:
                pnum = self.con.expect('Notification handle = .*? \r',
                                       timeout=4)
            except pexpect.TIMEOUT:
                print "TIMEOUT Exception!"
                break
        if pnum == 0:
            after = self.con.after
            hxstr = after.split()[3:]
            handle = long(float.fromhex(hxstr[0]))
            #try:
            if True:
                self.cb[handle]([long(float.fromhex(n)) for n in hxstr[2:]])
                #except:
                #  print "Error in callback for %x" % handle
                #  print sys.argv[1]
                pass
            else:
                print "TIMEOUT!!"
                pass

    def register_cb(self, handle, fn):
        self.cb[handle] = fn
        return

datalog = sys.stdout

def update(data):
    print(data)

def main():
    global datalog

    bluetooth_adr = sys.argv[1]
    #data['addr'] = bluetooth_adr
    if len(sys.argv) > 2:
        datalog = open(sys.argv[2], 'w+')

    #while True:
    try:
        pen = Device(bluetooth_adr)

        update_handle = pen.char_read_uuid("2902")[0]

        pen.register_cb(update_handle, update)

        pen.notification_loop()
    except:
        pass

if __name__ == "__main__":
    main()
