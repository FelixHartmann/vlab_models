from __future__ import absolute_import, print_function, unicode_literals, division

from multiprocessing import Pool
import subprocess
import sys


def process_file(i):
    with file("output{}.txt".format(i), "wb") as f:
        subprocess.call(['vveinterpreter', '--batch', 'model', str(i)], stdout=f, stderr=f)
        print("Process {} done".format(i))

if __name__ == '__main__':
    p = Pool(10)
    nb = int(sys.argv[1])
    p.map(process_file, range(nb))
    print("Done")
