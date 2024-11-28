from gem5.prebuilt.demo.x86_demo_board import X86DemoBoard
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator

board = X86DemoBoard()
board.set_workload(obtain_resource("x86-ubuntu-22.04-boot-no-systemd"))

sim = Simulator(board)
sim.run(20_000_000_000)
