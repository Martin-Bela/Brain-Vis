from typing import List
from math import sqrt
from random import random

import numpy as np
from scipy.cluster.hierarchy import linkage, dendrogram
from scipy.spatial.distance import pdist
import matplotlib.pyplot as plt

from loaders import load_positions, load_network, get_connexels, connex_size


def run_clustering(dir_path):
    positions = load_positions(dir_path)
    # POSITIONS LOADED
    network_60000 = load_network(dir_path, 60000)

    # now let's create 6D connection vector()

    connex = get_connexels(positions, network_60000)
    connex = list(filter(lambda c: connex_size(c) > 40, connex))

    print(len(connex))

    #distances = pdist(connex)
    distances = pdist(connex)


    L = linkage(distances, 'ward') # 'ward' method
    print(len(connex))
    fig = plt.figure(figsize=(25, 10))
    dn = dendrogram(L)
    #plt.show()

    