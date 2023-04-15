import argparse

from modules.playground import run_playground
from modules.clustering import run_clustering
from modules.project_to_2d import run_project_to_2d
from modules.timehist import run_timehist

def main():
    parser = argparse.ArgumentParser(
        prog="SciVis23Util",
        description="Utilities for preprocessing SciVIs23 files",
        epilog="Created for PA214 MUNI in 2023.")
    
    parser.add_argument('dir_name',
                        help="Directory in which we parse all things",
                        nargs='?',  # Sets to Optional 
                        default="../data/viz-calcium/")
    parser.add_argument('-c', '--clustering',
                        action='store_true',
                        help="Perform Agglomerative Clustering of network"
                        )
    parser.add_argument('-p', '--playground',
                        action='store_true',
                        help="Playground for debugging"
                        )
    parser.add_argument('-l', '--remove-local-connections',
                        action='store_true',
                        help="Remove Local Connections"
                        )
    parser.add_argument('-d', '--project-to-2d',
                        action='store_true',
                        help="Generate Projection  from 3D positions to 2D positions"
                        )
    parser.add_argument('-t', '--time-hist',
                        action='store_true',
                        help="Generate Histogram  Timeline"
                        )


    args = parser.parse_args()

    if args.clustering:
        print("clustering")
        run_clustering(args.dir_name)

    if args.playground:
        run_playground(args.dir_name)

    if args.project_to_2d:
        run_project_to_2d(args.dir_name)

    if args.time_hist:
        run_timehist(args.dir_name)

###############################################################################

if __name__ == "__main__":
    main()