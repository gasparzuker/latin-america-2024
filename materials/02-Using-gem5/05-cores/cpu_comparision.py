from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.private_l1_cache_hierarchy import PrivateL1CacheHierarchy
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator
from gem5.resources.resource import BinaryResource
from my_processor import BigCore, LittleCore, BigO3, LittleO3, BigProcessor,LittleProcessor

processor = BigProcessor()

###

cache = PrivateL1CacheHierarchy("64kB","64kB")

ram = SingleChannelDDR4_2400("2GB")

board = SimpleBoard("3GHz",processor,ram,cache)

board.set_se_binary_workload(binary=BinaryResource("matrix-multiply"))

sim = Simulator(board)

sim.run()

# with simple processor atomic and L1 Cache 32kB | SimInst = 33954546 | seconds = 0.012706
# with simple processor atomic and L1 Cache 64kB | SimInst = 33954546 | seconds = 0.012706

# with simple processor TIMING and L1 Cache 32kB| SimInst = 33954546 | Seconds = 0.020879
# with simple processor TIMING and L1 Cache 64kB | SimInst = 33954546 | seconds = 0.017881

# with simple processor MINOR and L1 Cache 32kB | SimInst = 33954565 | seconds = 0.015325
# with simple processor MINOR and L1 Cache 64kB | SimInst = 33954565 | seconds = 0.012378
# |_ a liitle bit more instructions, that's probably becouse minor processor has not some operations and need to replace them with combinations of other ones

# with simple processor O3 and L1 Cache 32kB| SimInst = 33954546 | Seconds = 0.009849
# with simple processor 03 and L1 Cache 64kB| SimInst = 33954546 | Seconds = 0.006883

#MY PROCESSORS
#Big processor    | SimInst = 33954546 | Seconds = 0.006883
#Little processor | SimInst = 33954546 | Seconds = 0.014725
