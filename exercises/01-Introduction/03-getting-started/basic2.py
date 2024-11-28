from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.cachehierarchies.ruby.mesi_two_level_cache_hierarchy import (
    MESITwoLevelCacheHierarchy,
)
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator

board.set_se_binary_workload(binary=BinaryResource("matrix-multiply"))

processor = SimpleProcessor(CPUTypes.TIMING,1,ISA.ARM)

cache = MESITwoLevelCacheHierarchy(l1i_size="32KiB",
    l1i_assoc=8,
    l1d_size="32KiB",
    l1d_assoc=8,
    l2_size="256KiB",
    l2_assoc=16,
    num_l2_banks=1
)

ram = SingleChannelDDR4_2400("2GB")

board = SimpleBoard("3GHz",processor,ram,cache)

board.set_workload(obtain_resource("arm-gapbs-bfs-run"))

sim = Simulator(board)

sim.run()
