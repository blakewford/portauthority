def steploop(debugger, unused0, unused1, unused2):
  target = debugger.GetSelectedTarget()
  process = target.GetProcess()
  thread = process.GetSelectedThread()
  frame = thread.GetFrameAtIndex(0)

  value = ""
  while str(value) != "No value":
    value = frame.FindRegister("pc")
    print(value);
    thread.StepInstruction( True )
