#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
from os import listdir
from os.path import isfile, join


class TheoreticalCpuCore():
    def __init__(self, id, online, physical_id):
        self.id = id
        self.online = online
        self.physical_id = physical_id

    def __str__(self):
        return 'Core {}: {}'.format(
            self.id,
            'online' if self.online else 'offline',
        )

    def enable(self):
        if not self.online:
            with open(
                    '/sys/devices/system/cpu/cpu' + self.id + '/online',
                    mode='w',
            ) as f:
                f.write('1')
            print(self.id + ': offline -> online')

    def disable(self):
        if self.online:
            with open(
                    '/sys/devices/system/cpu/cpu' + self.id + '/online',
                    mode='w',
            ) as f:
                f.write('0')
            print(self.id + ': online  -> offline')


class PhysicalCpuCore():
    def __init__(self, id, theoretical_cores=[]):
        self.id = id
        self.theoretical_cores = theoretical_cores

    def __str__(self):
        self.theoretical_cores.sort(key=lambda c: c.id)
        s = 'Physical Core {}:'.format(self.id)
        for t_core in self.theoretical_cores:
            s += '\n   ' + str(t_core)
        return s

    def disable_smt(self):
        self.theoretical_cores[0].enable()
        for core in self.theoretical_cores[1:]:
            core.disable()

    def enable_smt(self):
        for core in self.theoretical_cores:
            core.enable()


def parse_theoretical_cores():
    sys_dir = '/sys/devices/system/cpu/'
    cpu_regex = re.compile(r'cpu\d+')
    for directory in listdir(sys_dir):
        if cpu_regex.search(directory):
            try:
                with open(join(sys_dir, directory, 'online')) as f:
                    online = int(f.read().rstrip())
            except FileNotFoundError:
                online = 1
            if online:
                with open(join(sys_dir, directory, 'topology/core_id')) as f:
                    physical_id = int(f.read().rstrip())
            else:
                physical_id = 'unknown'

            yield TheoreticalCpuCore(directory[3:], online, physical_id)


def parse_physical_cores(theoretical_cores):
    physical_cores = dict()

    for t_core in theoretical_cores:
        pid = t_core.physical_id
        if pid in physical_cores:
            physical_cores[pid].theoretical_cores.append(t_core)
        else:
            physical_cores[pid] = PhysicalCpuCore(pid, [t_core])

    return [core for _, core in physical_cores.items()]


def main():
    theoretical_cores = [core for core in parse_theoretical_cores()]
    physical_cores = parse_physical_cores(theoretical_cores)
    physical_cores.sort(key=lambda c: c.id if isinstance(c.id, int) else 9999)

    print('\nStatus:')
    print('--------------')
    for core in physical_cores:
        print(str(core))

    CHOICE = input("Enter 'e' to enable or 'd' to disable SMT: ")

    if CHOICE == 'e':
        for core in physical_cores:
            core.enable_smt()
    elif CHOICE == 'd':
        for core in physical_cores:
            core.disable_smt()


if __name__ == '__main__':
    main()
