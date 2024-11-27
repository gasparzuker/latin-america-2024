"""
A simple run script using a specialized CHI cache hierarchy.
This script runs a simple test with a linear generator.

> gem5 run-test.py
"""

#from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import PrivateL1PrivateL2CacheHierarchy
from three_level import PrivateL1PrivateL2SharedL3CacheHierarchy

from gem5.components.boards.test_board import TestBoard
from gem5.components.memory.multi_channel import DualChannelDDR4_2400
from gem5.components.processors.linear_generator import LinearGenerator
from gem5.components.processors.random_generator import RandomGenerator

from gem5.simulate.simulator import Simulator

import argparse

p = argparse.ArgumentParser()
p.add_argument("testing", type=int, help="1 for L1 cahce, 2 for L2 cache, 3 for memory")
p = p.parse_args()

if (p.testing == 1):
    maxAdr = 32384
elif (p.testing == 2):
    maxAdr = 131072
elif (p.testing == 3):
    maxAdr = 1048576



### Tree levels


###



gen = RandomGenerator(num_cores=1, max_addr=maxAdr, rd_perc=75, duration="1ms")

board = TestBoard(
    #generator=LinearGenerator(num_cores=4, max_addr=2**22, rd_perc=75),
    generator=gen,
    cache_hierarchy=PrivateL1PrivateL2SharedL3CacheHierarchy(
        l1d_size="32KiB",
        l1d_assoc=2,
        l1i_size="32KiB",
        l1i_assoc=2,
        l2_size="256KiB",
        l2_assoc=2,
        l3_size="2MiB",
        l3_assoc=2
    ),
    memory=DualChannelDDR4_2400(size="2GB"),
    clk_freq="3GHz"
)

sim = Simulator(board)
sim.run()

# l1d Cache missRate::total = 0.000182 (read) 0.000174 (write)
# l1i Cache missRate::total = ?


# l2 cache missRate::total = 0.003257 (read) 0.002793 (invalidReq ?)


# The miss rate of previous levels on this tests are lower than (almost) 100, more close to 75/85
# I believe this is becouse of the maxAdr, if I make it bigger the results are more close to 95

