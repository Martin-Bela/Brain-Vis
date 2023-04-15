from typing import List
from math import sqrt
from random import random

import numpy as np
from scipy.cluster.hierarchy import linkage, dendrogram
from scipy.spatial.distance import pdist

import matplotlib.pyplot as plt

from loaders import load_positions, load_data_summary

def distance(A: List[int], B: List[int]):
    dx = A[0] - B[0]
    dy = A[1] - B[1]
    return sqrt(dx*dx + dy*dy)


def run_playground(dir_path):
    ds = load_data_summary(dir_path + "monitors-histogram/electricActivity.txt")
    ds2 = list(map(lambda t: t[3], ds))
    print(ds)
    plt.plot(ds2)
    plt.show()


    pass