import re

def manhattan(x, y, z, px, py, pz):
    dx = x - px
    dy = y - py
    dz = z - pz
    return abs(dx) + abs(dy) + abs(dz)


def create_map(folder):
    file = folder + "positions\\rank_0_positions.txt"

    point_index = 0
    added_index = -1
    px, py, pz = 0, 0, 0
    point_map = {}
    with open(file) as f:
        for line in f:
            if line[0] == "#":
                continue

            split_data = re.split(' |\t', line)
            x = float(split_data[1])
            y = float(split_data[2])
            z = float(split_data[3])

            if added_index == -1 or manhattan(x, y, z, px, py, pz) > 0.5:
                added_index = point_index
                px = x
                py = y
                pz = z
            point_map[point_index] = added_index
            point_index += 1
    return point_map


def process_monitors(iterations, folder):
    point_map = create_map(folder)
    chosen_neurons = sorted(list(set(point_map.values())))

    for iteration in range(0, iterations * 10000, 10000):
        print(iteration)
        
        result = []

        for neuron in chosen_neurons:
            file = folder + "monitors\\0_" + str(neuron) + ".csv"
            with open(file) as f:
                result.append(f.readlines()[iteration // 100])
        f = open(folder + "monitors2\\monitors_" + str(iteration) + ".csv", "w")
        f.writelines(result)
        f.close()
        print("Finished iteration #", str(iteration))
        

if __name__ == "__main__":
    process_monitors(100, ".\\data\\viz-calcium\\")