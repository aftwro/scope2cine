
/* CarDV_DispToHdmiTiming(MI_DISP_OutputTiming_e) */

undefined4 CarDV_DispToHdmiTiming(int param_1)

{
  int *piVar1;
  int iVar2;
  undefined1 *puVar3;
  
  iVar2 = 0;
  puVar3 = astTimingArray;
  do {
    piVar1 = (int *)(puVar3 + 0x20);
    puVar3 = puVar3 + 0x28;
    if (*piVar1 == param_1) {
      return *(undefined4 *)(astTimingArray + iVar2 * 0x28 + 0x24);
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 != 0x1a);
  return 0x31;
}

