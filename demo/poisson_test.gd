extends Node

const GRAPH_RESULT_MAX: int = 40
const GRAPH_DATA_LABEL_A: String = "COUNT"
const GRAPH_DATA_LABEL_B: String = "NUMBER"

enum TestPhase { START, SMALL, END, EXIT }
const MAX_ITER: int = 500

var rng = RandomNumberGenerator.new()
var curr_iter: int = 0
var results_map: Dictionary = {}

var phase: TestPhase = TestPhase.START
var phase_msec: int

func _init():
    print("🐠 Poisson v1.0")
    print("🐠 Starting tests: %s iterations per test" % MAX_ITER)

func _process(_delta: float):
    if (phase == TestPhase.START):
        print("🐠 Starting small lambda test")
        phase = TestPhase.SMALL
        phase_msec = Time.get_ticks_msec()

    if (phase == TestPhase.SMALL):
        if (curr_iter < MAX_ITER):
            var dist: int = rng.rand_poisson(15)
            var count: int = results_map.get(dist, 0)

            results_map[dist] = count + 1
            curr_iter += 1
        elif (curr_iter == MAX_ITER):
            phase_msec = Time.get_ticks_msec() - phase_msec
            print("🐠 Finished small lambda test in %.2fms" % phase_msec)

            print_results_graph()

            curr_iter = 0
            phase = TestPhase.END

    if (phase == TestPhase.END):
        print("🐠 All done!")
        phase = TestPhase.EXIT
        phase_msec = 0


# print_results_graph
#
# Quick program to generate console graphs
#   NOTE: Depends on `results_map`
func print_results_graph():
    var results: Array = results_map.keys()
    results.sort()

    var results_pad_length: int = str(results[results.size() - 1]).length()
    results_pad_length = results_pad_length if results_pad_length % 2 == 0 else results_pad_length + 1
    results_pad_length = maxi(results_pad_length + 1, len(GRAPH_DATA_LABEL_B) + 1)

    var counts: Array = results_map.values()
    counts.sort()

    var counts_pad_length: int = str(counts[counts.size() - 1]).length()
    counts_pad_length = counts_pad_length if counts_pad_length % 2 == 0 else counts_pad_length + 1
    counts_pad_length = maxi(counts_pad_length + 1, len(GRAPH_DATA_LABEL_A) + 1)

    print("%*s%*s" % [
        counts_pad_length,
        GRAPH_DATA_LABEL_A,
        results_pad_length,
        GRAPH_DATA_LABEL_B])
    for result in results:
        var count: int = results_map[result]
        print("%*s%*s    %s" % [
            counts_pad_length,
            "%d" % count,
            results_pad_length,
            "%d" % result,
            "*".repeat(floor((count * 1.0 / MAX_ITER) * GRAPH_RESULT_MAX))])
