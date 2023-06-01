from loaders import load_monitors_bin
import numpy as np
import matplotlib.pyplot as plt 

from pathlib import Path

from neuron_properties import *



NEURONS_COUNT = 50000
MAX_STEPS = 10000

STEPS = MAX_STEPS

# ATTRIBUTES = [
#     FIRED,
#     FIRED_FRACTION,
#     ELECTRIC_ACTIVITY, 
#     SECONDARY_VARIABLE,
#     CALCIUM,
#     TARGET_CALCIUM,
#     SYNAPTIC_INPUT,
#     BACKGROUND_ACTIVITY,
#     GROWN_AXONS,
#     CONNECTED_AXONS,
#     GROWN_DENDRITES,
#     CONNECTED_DENDRITES,
# ]
ATTRIBUTES = [
     FIRED,
]


def fix_fired_bug(timestep):
    """ Fixing the bug in preprocessing where fires is 48 if false and 49 if true
    """
    for i, neurons in enumerate(timestep):
        timestep[i] = tuple([neurons[0]-48]) + neurons[1:] 


def get_timestep_hist(dir_path, timestep, attributes):
    timestep_tuples = load_monitors_bin(dir_path, timestep)
    fix_fired_bug(timestep_tuples)

    # Convert array of tuples to array of  arrays (for each tuple one array)
    properties = list(map(list, (zip(*timestep_tuples))))

    histograms = []
    for i in attributes:
        np_property = np.array(properties[i])

        #range = (NEURON_PROPERTIES[i].min, NEURON_PROPERTIES[i].max)
        hist = np.histogram(np_property, bins = NEURON_PROPERTIES[i].bins_count)

        histograms.append(hist)
    
    return tuple(histograms)

def run_timehist(dir_path):

    # here we are initializing max, min variables
    init_neuron_properties(dir_path)

    # time_hist structure is :
    # [[(timestep0-attr0, timestep0-attr1)], [(timestep1-attr0, timestep1-attr1)]... ]
    time_hist = []
    for i in range(0, STEPS):
        print(f"{int(i * 100 / STEPS)}%")
        time_step_histograms = get_timestep_hist(dir_path, i,  ATTRIBUTES)
        time_hist.append(time_step_histograms)

    # Now we have np.histograms for every attribute we wanted
    # Cunvert array of tuples to array of  arrays (for each tuple one array)
    histograms = list(map(list, (zip(*time_hist))))

    for i in range(len(histograms)):
        attribute_histogram = histograms[i]
        neuron_property = NEURON_PROPERTIES[ATTRIBUTES[i]]
        histogram, values = tuple(map(list, (zip(*attribute_histogram))))

        stacked= np.stack(histogram)

        # Solve by this very bad code
        #if i == FIRED:
        #    print(stacked,)

        Path(f"{dir_path}monitors-hist-real2").mkdir(parents=True, exist_ok=True)
        np.savetxt(f"{dir_path}monitors-hist-real2/{neuron_property.filename}.txt",   stacked, delimiter=" ", fmt='%i')

        stacked = np.rot90(stacked)
        plt.imshow(stacked, cmap='hot')
        plt.title(neuron_property.name)

    #print(np.stack(histograms))
