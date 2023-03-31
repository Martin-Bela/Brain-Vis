import argparse

from modules.playground import run_playground
from modules.clustering import run_clustering

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


    args = parser.parse_args()
    print(args)
    if args.clustering:
        print("clustering")
        run_clustering(args.dir_name)

    if args.playground:
        run_playground(args.dir_name)

###############################################################################

if __name__ == "__main__":
    main()