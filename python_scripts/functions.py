from typing import List, Optional, Dict
from math import sqrt

import numpy as np

def debug_print():
    print('debug')


def load_positions(dir_path):
    """Load Positions type of File From SciVis23.
        Type of output: list(id: int, x: float, y: float, z: float, area:str[7], ?: s[2]))


    Args:
        dir_path (_type_): 

    Returns:
        np.array: Positions Loaded
    """
    positions = np.loadtxt(dir_path + "positions/rank_0_positions.txt",
                                delimiter=" ",
                                comments="#",
                                dtype={
                                    'names': ('id', 'x', 'y', 'z', 'area', '??'),
                                    'formats': ('i', 'f', 'f', 'f', 'S7', 'S2')
                                })
    return positions        


    
def load_network(dir_path: str, step :int, type_of="in"):
    """Load Network type of File From SciVis23.

        TypeOfOutput (id: int, t_id: int, t_rank: int, s_id: int, s_rank: int)
    Args:
        dir_path (str): path to given directory (no-netowrk, calcium etc...) 
        step (int): which step (We need to add the way to load more depending on data
        type (str, optional): type of file (there is in/out). Defaults to "in".

    Returns:
        np.array: Network Loaded
    """
    network = np.loadtxt(dir_path + f"network/rank_0_step_{str(step)}_{type_of}_network.txt",
                                comments="#",
                                dtype={
                                    'names': ('id','target_id', 'targed_rank', 'source_id', 'source_rank'),
                                    'formats': ('i', 'i', 'i', 'i', 'i')
                                })
    return network
    

def load_networks(dir_path: str, steps: list[int], type_of="in"):
    networks = []

    for step in steps:
        networks.append(load_network(dir_path, step, type_of))
        
    return networks


def get_connexels(positions, network):
    """Gets connexels from positions and from network

    Args:
        positions (list): List of Positions loaded by load_positions
        network (list): List of Network loaded by load_network
        
    Returns:
        np.array: Connexels created
    """

        # now let's create 6D connection vector()
    connex = []

    for con in network:
        t_id = con[1]-1
        s_id = con[3]-1
        px = positions[s_id][1]
        py = positions[s_id][2]
        pz = positions[s_id][3]
        qx = positions[t_id][1]
        qy = positions[t_id][2]
        qz = positions[t_id][3]
        connex.append((px, py, pz, qx, qy, qz, 1))
    return connex


def connex_size(c):
    dx = c[0] - c[3]
    dy = c[1] - c[4]
    dz = c[2] - c[5]
    return sqrt(dx*dx + dy*dy + dz*dz)


def load_moitors():
    