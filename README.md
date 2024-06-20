# Brain Visualisation
![brain image](screenshot.jpg?raw=true "Screenshot of the application")

Required vcpkg libraries:
vtk[core, qt]

## How to preprocess data

1. Download Data from [Sci Vis 2023 Page](https://sciviscontest2023.github.io/data/#Download%20Data)
2. unpack archive to `data/viz-calcium  data/viz-disable  data/viz-no-network  data/viz-stimulus` in parent git directory.
3. Unload `monitors.zip` into `monitors` directory.
4. Compile and Run `PreprocessNeuronProperties` and `PreprocessNetwork` target.
5. Preprocess histogram data by running `python3 python_scripts/main.py -t`
6. Now all the preprocessing should be done and you can compile and run `Brain Visualization` target.
