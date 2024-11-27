"""
This script creates a simple system with a traffic generator to test memory

$ gem5 memory-test.py
"""

import argparse

from gem5.components.boards.test_board import TestBoard
from gem5.components.cachehierarchies.classic.no_cache import NoCache
from gem5.components.memory.simple import SingleChannelSimpleMemory
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.memory.multi_channel import ChanneledMemory
from gem5.components.memory.dram_interfaces.lpddr5 import LPDDR5_6400_1x16_BG_BL32
from gem5.components.processors.linear_generator import LinearGenerator
from gem5.components.processors.random_generator import RandomGenerator
from gem5.simulate.simulator import Simulator

parser = argparse.ArgumentParser()
parser.add_argument("rate", type=str, help="The rate of the generator in GiB/s")
parser.add_argument("rd_perc", type=int, help="The rate of the generator, number between 0 and 100 (represents porcentage)")
parser.add_argument("random", type=str, help="Linear or random generator")
parser.add_argument("memtype", type=str, help="TypeOfMemory")

args = parser.parse_args()

if (args.random == "random"):
    generator=RandomGenerator(
        num_cores=1,
        rate=args.rate+"GiB/s",
        rd_perc=args.rd_perc)
else:
    generator=LinearGenerator(
        num_cores=1,
        rate=args.rate + "GiB/s",
        rd_perc=args.rd_perc)

if (args.memtype == "ddr4"):
    memory = SingleChannelDDR4_2400("1GiB")
elif (args.memtype == "ddr5"):
    memory = ChanneledMemory(LPDDR5_6400_1x16_BG_BL32, 4, 64)
else:
    memory=SingleChannelSimpleMemory(
        "20ns",
        "0s",
        bandwidth="32GiB/s",
        size="1GiB")

board = TestBoard(
    clk_freq="3GHz", #ignored


    generator=generator,

    memory=memory,

    cache_hierarchy=NoCache()
)

sim = Simulator(board=board)
sim.run()

#With Bandwith of 32GiB/s -> read bandwith 17180800000 (17.8 GB/s)

#16 - 50 = 8500224000 + 8680512000
#32 - 50 = 17019584000 + 17332736000
#64 - 50 = 17082739375.855600 + 17399219141.977051

#16 - 100 = 17180800000
#32 - 100 = 34352320000
#64 - 100 = 34481958517.832649

#Segunda parte

#16 - 100 - Random - DDR4 = 16725440000
#16 - 100 - Linear - DDR4 = 16725440000

#32 - 100 - Random - DDR4 = 18240741149.629028

#32 - 50 - Linear  - DDR4 = 9803520000 + 9968128000

#32 - 50 - Random - DDR5 = 5556329833.459548 + 5664676409.304921
# 32 - 100 - Linear - DDR5 = 11916753608.078203
#32 - 50 - Linear - DDR5 4 chan = 4347776000 * 2 = 8603328000
