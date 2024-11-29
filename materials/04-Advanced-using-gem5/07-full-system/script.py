from gem5.components.boards.x86_board import X86Board
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.cachehierarchies.classic.private_l1_shared_l2_cache_hierarchy import PrivateL1SharedL2CacheHierarchy
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.simulate.simulator import Simulator
from gem5.simulate.exit_event import ExitEvent
from gem5.resources.workload import obtain_resource
from gem5.components.boards.x86_board import X86Board

processor = SimpleProcessor(isa=ISA.X86, num_cores=4, cpu_type=CPUTypes.KVM)
memory = DualChannelDDR4_2400("2GB")
cache = PrivateL1SharedL2CacheHierarchy(l1d_size="32KiB",l1d_assoc=2,l1i_size="32KiB",l1i_assoc=2,l2_size="128KiB",l2_assoc=2)
board = X86Board("3GHz", processor, memory, cache)

# workload = obtain_resource("x86-ubuntu-22.04-boot-with-systemd")

# board.set_workload(workload)
# board.append_kernel_arg("interactive=true")

board.set_kernel_disk_workload(
    kernel = obtain_resource("x86-linux-kernel-5.4.0-105-generic"),
    disk_image = obtain_resource("x86-ubuntu-22.04-img"),
    kernel_args = ["earlyprintk=ttyS0", "console=ttyS0", "lpj=7999923", "root=/dev/sda2"],
    readfile_contents = "echo 'Hello, world!'; sleep 5",

)




for proc in processor.cores:
    proc.core.usePerf = False

def exit_event_handler():
    print("first exit event: Kernel booted")
    yield False
    print("second exit event: In after boot")
    yield False
    print("third exit event: After run script")
    yield True



simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.EXIT: exit_event_handler(),
    },
)
simulator.run()
