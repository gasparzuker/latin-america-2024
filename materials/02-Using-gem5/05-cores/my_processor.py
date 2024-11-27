from m5.objects import RiscvO3CPU
from gem5.components.processors.base_cpu_core import BaseCPUCore
from  gem5.components.processors.base_cpu_processor import BaseCPUProcessor
from gem5.isas import ISA

# RiscvO3CPU specialized models
class BigO3(RiscvO3CPU):
    def __init__(self):
        super().__init__()

        # Widths
        self.fetchWidth = 8
        self.decodeWidth = 8
        self.renameWidth = 8
        self.issueWidth = 8
        self.wbWidth = 8

        # Regs
        self.numROBEntries = 256
        self.numPhysIntRegs = 512
        self.numPhysFloatRegs = 512

class LittleO3(RiscvO3CPU):
    def __init__(self):
        super().__init__()

        # Widths
        self.fetchWidth = 2
        self.decodeWidth = 2
        self.renameWidth = 2
        self.issueWidth = 2
        self.wbWidth = 2

        # regs
        self.numROBEntries = 30
        self.numPhysIntRegs = 40
        self.numPhysFloatRegs = 40



# Cores using the models
class BigCore(BaseCPUCore):
    def __init__(self):
        super().__init__(BigO3(), ISA.RISCV)



class LittleCore(BaseCPUCore):
    def __init__(self):
        super().__init__(LittleO3(), ISA.RISCV)



# Full processors using the cores
class BigProcessor(BaseCPUProcessor):
    def __init__(self):
        super().__init__(cores=[BigCore()])

class LittleProcessor(BaseCPUProcessor):
    def __init__(self):
        super().__init__(cores=[LittleCore()])
