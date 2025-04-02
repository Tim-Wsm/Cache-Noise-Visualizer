#!/usr/bin/env python3
# -*- coding: utf-8 -*-

with open('/proc/sys/vm/nr_hugepages') as f:
    nr_hugepages = f.read()
    print('Current number of huge pages: ' + nr_hugepages)

print('To profile the cache a minimum of one hugepage is needed.')
new_nr = input('New Number: ')

if int(new_nr) < 0:
    print('no negative hugepage numbers are allowed')
    exit(1)

with open('/proc/sys/vm/nr_hugepages', mode='w') as f:
    f.write(new_nr)
