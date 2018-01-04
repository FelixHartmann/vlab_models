from __future__ import division, print_function
import numpy as np
import pandas as pd
from path import path
from violin import violin

names = ['1cell', '2cell', '4cell',
         'Apical', 'Basal',
         'InnerApical', 'OuterApical', 'InnerBasal', 'OuterBasal']


def load_data(name):
    fs = path('.').files('{}_sizes*.csv'.format(name))
    data = {n: [] for n in names}
    for f in fs:
        d = pd.read_csv(f)
        for n in names:
            data[n].append(d.Volume[d.Cell == n].values)
    for n in names:
        data[n] = np.concatenate(data[n])
    return data


def plot(data):
    return violin([data[n] for n in names], labels=names)
