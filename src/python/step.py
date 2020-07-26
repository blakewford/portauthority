import lldb
import binascii

def steploop(debugger, unused0, unused1, unused2):
  target = debugger.GetSelectedTarget()
  process = target.GetProcess()
  thread = process.GetSelectedThread()
  frame = thread.GetFrameAtIndex(0)

  value = ""
  while str(value) != "No value":
    value = frame.FindRegister("pc")
    if str(value) != "No value":
      error = lldb.SBError()
      bytes = process.ReadMemory(value.GetValueAsUnsigned(), 8, error)
      thread.StepInstruction( True )
      print(hex(value.GetValueAsUnsigned()) + " : 0x" + str(binascii.hexlify(bytes))[2:-1])
    
