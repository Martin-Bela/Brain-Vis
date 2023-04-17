from typing import List
from math import sqrt
from random import random

import numpy as np
from scipy.cluster.hierarchy import linkage, dendrogram
from scipy.spatial.distance import pdist

import matplotlib.pyplot as plt

from loaders import load_positions, load_data_summary
from neuron_properties import *

def distance(A: List[int], B: List[int]):
    dx = A[0] - B[0]
    dy = A[1] - B[1]
    return sqrt(dx*dx + dy*dy)

MEAN = 0
SUM = 1
MAX = 2
MIN = 3
SUMMARIES = [ 'mean', "sum", "max", "min" ]

def run_playground(dir_path):

    for prop in NEURON_PROPERTIES:
        ds = load_data_summary(dir_path + f"monitors-histogram/{prop.filename}.txt")
        # for i, sum in enumerate(SUMMARIES):
        #     ds2 = list(map(lambda t: t[i], ds))
        #     plt.plot(ds2)
        #     plt.title(f"{prop.name} - {sum}")
        #     plt.savefig(f"../figs/{prop.filename}_{sum}")
        #     plt.show()

        dsmin = list(map(lambda t: t[MIN], ds))
        dsmax = list(map(lambda t: t[MIN], ds))
        dsmean = list(map(lambda t: t[MIN], ds))

        ds_all = list(map(lambda t: (t[MAX], t[MEAN], t[MIN]), ds))
        plt.plot(ds_all)
        plt.title(f"{prop.name} - ALL")
        plt.savefig(f"../figs/{prop.filename}_z_all")
        plt.show()
        
        
            


    pass