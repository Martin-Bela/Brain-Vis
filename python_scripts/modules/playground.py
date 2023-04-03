from typing import List
from math import sqrt
from random import random

import numpy as np
from scipy.cluster.hierarchy import linkage, dendrogram
from scipy.spatial.distance import pdist

import matplotlib.pyplot as plt

from functions import load_positions

def distance(A: List[int], B: List[int]):
    dx = A[0] - B[0]
    dy = A[1] - B[1]
    return sqrt(dx*dx + dy*dy)


def run_playground(dir_path):
    positions = load_positions(dir_path)
    pos_only = list(map(lambda X: (X[1], X[2], X[3]), positions))

    ddists = []
    for i in range(len(pos_only)-1):
        j = i + 1
        p1 = pos_only[j]
        p0 = pos_only[i]

        dx = p1[0] - p0[0]
        dy = p1[1] - p0[1]
        dz = p1[2] - p0[2]

        ddist = sqrt(dx*dx + dy*dy + dz*dz)
        ddists.append(ddist)

    ddists = np.array(ddists)
    #plt.hist(ddist, bins=100)
    plt.plot(ddists)
    plt.show()    