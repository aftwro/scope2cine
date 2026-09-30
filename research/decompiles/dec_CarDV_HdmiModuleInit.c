
/* CarDV_HdmiModuleInit(MI_HDMI_TimingType_e) */

int CarDV_HdmiModuleInit(undefined4 param_1)

{
  int iVar1;
  undefined1 *local_50;
  undefined4 uStack_4c;
  char local_48 [4];
  undefined1 local_44;
  undefined4 local_40;
  undefined4 local_30;
  
  local_50 = &LAB_00030f34_1;
  uStack_4c = 0;
  iVar1 = MI_HDMI_Init(&local_50);
  if (iVar1 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_HdmiModuleInit",0x6c,iVar1);
    return iVar1;
  }
  iVar1 = MI_HDMI_Open();
  if (iVar1 == 0) {
    memset(local_48,0,0x30);
    local_44 = 1;
    local_30 = 4;
    local_40 = param_1;
    iVar1 = MI_HDMI_SetAttr(0,local_48);
    if (iVar1 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_HdmiModuleInit",0x7f,iVar1);
      return iVar1;
    }
    iVar1 = MI_HDMI_GetAttr(0,local_48);
    if (iVar1 == 0) {
      if (local_48[0] != '\0') {
        iVar1 = MI_HDMI_Start();
        if (iVar1 == 0) {
          bHdmiStart = 1;
        }
        else {
          printf("(%s %d)exec func fail, err:%x\n","CarDV_HdmiModuleInit",0x83,iVar1);
        }
      }
    }
    else {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_HdmiModuleInit",0x80,iVar1);
    }
  }
  else {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_HdmiModuleInit",0x6d,iVar1);
  }
  return iVar1;
}

