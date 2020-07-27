import lldb
import binascii

def steploop(debugger, unused0, unused1, unused2):
  target = debugger.GetSelectedTarget()
  process = target.GetProcess()
  thread = process.GetSelectedThread()

  value = ""
  while str(value) != "No value":
    frame = thread.GetFrameAtIndex(0)
    value = frame.FindRegister("pc")
    if str(value) != "No value":
      error = lldb.SBError()
      address = value.GetValueAsUnsigned()
      bytes = process.ReadMemory(address, 8, error)
      print(hex(address) + " : 0x" + str(binascii.hexlify(bytes))[:-1])
#      if (address >= 0x400520) and (address <= 0x40053c): .plt
#        thread.StepOut()
#      else:
      thread.StepInstruction(False)
