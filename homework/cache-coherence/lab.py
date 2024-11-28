from components.boards import HWX86Board
from components.cache_hierarchies import HWMESITwoLevelCacheHierarchy
from components.memories import HWDDR4, DDR4_2400_8x8
from components.processors import HWO3CPU
from workloads.roi_manager import exit_event_handler
from gem5.simulate.simulator import Simulator
from gem5.resources.resource import BinaryResource
from gem5.resources.workload import obtain_resource
#from components.network import L1L2ClusterTree
from os import getcwd

import argparse

print(getcwd())
parser = argparse.ArgumentParser()

parser.add_argument("resource", type=str)
parser.add_argument("latency", type=int)
parser.add_argument("cores", type=int)

args = parser.parse_args()


ram = HWDDR4()

cache = HWMESITwoLevelCacheHierarchy(args.latency)

cpu = HWO3CPU(args.cores)

board = HWX86Board(clk_freq="3GHz",processor=cpu,memory=ram, cache_hierarchy=cache)

board.set_se_binary_workload(BinaryResource("/workspaces/latin-america-2024/homework/cache-coherence/array_sum/chunking-gem5"))

sim = Simulator(board=board, full_system=False, on_exit_event=exit_event_handler)

sim.run()
