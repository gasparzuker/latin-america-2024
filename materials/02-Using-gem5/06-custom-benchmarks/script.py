from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.cachehierarchies.classic.private_l1_shared_l2_cache_hierarchy import PrivateL1SharedL2CacheHierarchy
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator, ExitEvent
import m5.stats

import argparse

##########
def workbegin_handler():
    print("Workbegin handler")
    m5.stats.dump()
    m5.stats.reset()
    yield False

def workend_handler():
    print("Workend handler")
    m5.stats.dump()
    m5.stats.reset()
    yield False
##########

parser = argparse.ArgumentParser()

parser.add_argument("ISA", type=str, help="x86 or riscv")
args = parser.parse_args()

if(args.ISA == "x86"):
    processor = SimpleProcessor(CPUTypes.TIMING,1,ISA.X86)
elif (args.ISA == "riscv"):
    processor = SimpleProcessor(CPUTypes.TIMING,1,ISA.RISCV)


#cache = MESITwoLevelCacheHierarchy(l1i_size="32KiB",
#    l1i_assoc=8,
#    l1d_size="32KiB",
#    l1d_assoc=8,
#    l2_size="256KiB",
#    l2_assoc=16,
#    num_l2_banks=1
#)

cache = PrivateL1SharedL2CacheHierarchy(
    l1d_size="64kB", l1i_size="64kB", l2_size="1MB",
)

ram = SingleChannelDDR4_2400("2GB")

board = SimpleBoard("3GHz",processor,ram,cache)

if(args.ISA == "x86"):
    board.set_workload(obtain_resource("matrix-multiply"))
elif(args.ISA == "risv"): #Error
    board.set_workload(obtain_resource("matrix-multiply"))




sim = Simulator(board=board,
    on_exit_event={
        ExitEvent.WORKBEGIN: workbegin_handler(),
        ExitEvent.WORKEND: workend_handler()
    }
)


sim.run()

#x86 | Time          |
#PRE |
#ROI |
#POST|

#RISK| Time          | Instructions |
#PRE | 0.027529      |
#ROI |
#POST|
