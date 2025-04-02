#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
If this file is 1, then direct user-space access to the performance counter
registers is allowed via the rdpmc instruction.  This can be disabled by echoing
0 to the file.

As of Linux 4.0 the behavior has changed, so that 1 now means only allow access
to processes with active perf events, with 2 indicating the old
allow-anyone-access behavior.
"""

with open('/sys/bus/event_source/devices/cpu/rdpmc') as f:
    current_value = f.read()

text = """Possible states regarding the rdpmc instruction:
    0: disabled
    1: only accessable via active perf events
    2: allow access for anyone (recommended)

Current Value: {}""".format(current_value)

print(text)

new_val = input('New Value: ')

if not (0 <= int(new_val) <= 2):
    print('the new value has to be between 0 and 2')
    exit(1)

with open('/sys/bus/event_source/devices/cpu/rdpmc', mode='w') as f:
    f.write(new_val)
