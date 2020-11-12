import lldb
import binascii
from datetime import datetime

def steploop(debugger, unused0, unused1, unused2):
  target = debugger.GetSelectedTarget()
  process = target.GetProcess()
  thread = process.GetSelectedThread()
  module = target.GetModuleAtIndex(0)

  linkTable = module.FindSection(".plt")
  tableAddress = linkTable.GetFileAddress()

  mnem = ""
  value = ""
  replay = ""
  while str(value) != "No value":
    frame = thread.GetFrameAtIndex(0)
    value = frame.FindRegister("pc")
    if str(value) != "No value":
      error = lldb.SBError()
      address = value.GetValueAsUnsigned()
      bytes = process.ReadMemory(address, 8, error)
      mnem = target.GetInstructions(lldb.SBAddress(value.GetLoadAddress(), target), bytes).GetInstructionAtIndex(0).GetMnemonic(target)
      replay += hex(value.GetValueAsUnsigned()) + " : 0x" + str(binascii.hexlify(bytes))[:-1]
      replay += " " + str(mnem)
      replay += "\n"
      if (address >= tableAddress) and (address <= tableAddress + linkTable.GetByteSize()):
        thread.StepOut()
      else:
        thread.StepInstruction(False)

  f = open(str(datetime.now()).replace(' ',''), "w")
  f.write(replay)
  f.close()
