from loaders import load_data_summary

INF_PLUS= float("inf") 
INF_MINUS = float("-inf")

class NeuronProperties:
    def __init__(self, id, name, filename, bins_count) -> None:
        self.id = id
        self.name = name
        self.filename = filename
        self.bins_count = bins_count
        self.min = 0
        self.max = 64


FIRED = 0 
FIRED_FRACTION = 1
ELECTRIC_ACTIVITY = 2
SECONDARY_VARIABLE = 3
CALCIUM = 4
TARGET_CALCIUM = 5
SYNAPTIC_INPUT = 6
BACKGROUND_ACTIVITY = 7
GROWN_AXONS = 8
CONNECTED_AXONS = 9
GROWN_DENDRITES = 10
CONNECTED_DENDRITES = 11


NEURON_PROPERTIES = [
    NeuronProperties(FIRED, "Fired", "fired", 2),
    NeuronProperties(FIRED_FRACTION,"Fired Fraction",   "firedFraction", 64),
    NeuronProperties(ELECTRIC_ACTIVITY, "Electric Activity", "electricActivity", 64),
    NeuronProperties(SECONDARY_VARIABLE, "Secondary Variable", "secondaryVariable", 64),
    NeuronProperties(CALCIUM, "Calcium", "calcium", 64),
    NeuronProperties(TARGET_CALCIUM, "Target Calcium", "targetCalcium", 64),
    NeuronProperties(SYNAPTIC_INPUT, "Synaptic Input", "synapticInput", 64),
    NeuronProperties(BACKGROUND_ACTIVITY, "Background Activity", "backgroundActivity", 64),
    NeuronProperties(GROWN_AXONS, "Grown Axons", "grownAxons", 64),
    NeuronProperties(CONNECTED_AXONS, "Connected Axons", "connectedAxons", 64),
    NeuronProperties(GROWN_DENDRITES, "Grown Dendrites", "grownDendrites", 64),
    NeuronProperties(CONNECTED_DENDRITES, "Connected Dendrites", "connectedDendrites", 64)
]


"""
In C++ called histograms but it is clashing with meaning of histogram in 
this script so i call it data_summary
"""
def init_neuron_properties(dir_path):
    print(f"Initializing Neuron Properties (min, max) from {dir_path}")

    for neuronProperties  in NEURON_PROPERTIES:
        path = dir_path + "monitors-histogram/" + neuronProperties.filename + ".txt"
        data = load_data_summary(path)
        max_of_all = INF_MINUS
        min_of_all = INF_PLUS
        for timestep in data:
            min_of_all = min(timestep["min"], min_of_all)
            max_of_all = max(timestep["max"], max_of_all)
        neuronProperties.min = min_of_all
        neuronProperties.max = max_of_all
    print(f"Initialization of Neuron Properties Finished")