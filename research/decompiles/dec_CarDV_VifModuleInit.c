
/* CarDV_VifModuleInit(unsigned int) */

int CarDV_VifModuleInit(uint param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  int local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 uStack_5c;
  undefined4 local_58;
  undefined1 auStack_54 [4];
  int local_50;
  undefined4 local_2c;
  
  iVar2 = param_1 * 0x178;
  uVar4 = *(undefined4 *)(gstVifModule + iVar2 + 4);
  memset(auStack_54,0,0x34);
  local_6c = 0;
  local_70 = 0;
  local_68 = 0;
  local_64 = 0;
  local_60 = 0;
  uStack_5c = 0;
  local_58 = 0;
  if (*(short *)(gstVifModule + iVar2) == 0x100) {
    iVar1 = MI_SNR_GetPadInfo(uVar4,auStack_54);
    if (iVar1 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_VifModuleInit",0x18b,iVar1);
      return iVar1;
    }
    local_6c = *(undefined4 *)(gstVifModule + iVar2 + 0x1c);
    local_68 = *(undefined4 *)(gstVifModule + iVar2 + 0x20);
    local_70 = local_50;
    if (local_50 != 0) {
      local_2c = 2;
    }
    local_64 = local_2c;
    if (gstVifModule[param_1 * 0x178 + 0x18] == '\0') {
      iVar2 = MI_VIF_CreateDevGroup(param_1,&local_70);
      if (iVar2 != 0) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_VifModuleInit",0x19e,iVar2);
        return iVar2;
      }
    }
    else {
      iVar2 = MI_VIF_CreateDevGroupExt(param_1,&local_70,param_1 * 0x178 + 0x7af10);
      if (iVar2 != 0) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_VifModuleInit",0x19a,iVar2);
        return iVar2;
      }
    }
    gstVifModule[param_1 * 0x178] = 1;
  }
  uVar3 = param_1 * 4;
  if (*(short *)(gstVifModule + param_1 * 0x178 + 0x24) == 0x100) {
    iVar2 = CarDV_VifDevInit(uVar3);
    if (iVar2 == 0) {
      gstVifModule[param_1 * 0x178 + 0x24] = 1;
      goto LAB_00027124;
    }
LAB_0002723e:
    printf("(%s %d)exec func fail, err:%x\n","CarDV_VifModuleInit",0x1a9,iVar2);
  }
  else {
LAB_00027124:
    if (*(short *)(gstVifModule + param_1 * 0x178 + 0x78) == 0x100) {
      iVar2 = CarDV_VifDevInit(uVar3 + 1);
      if (iVar2 != 0) goto LAB_0002723e;
      gstVifModule[param_1 * 0x178 + 0x78] = 1;
    }
    if (*(short *)(gstVifModule + param_1 * 0x178 + 0xcc) == 0x100) {
      iVar2 = CarDV_VifDevInit(uVar3 + 2);
      if (iVar2 != 0) goto LAB_0002723e;
      gstVifModule[param_1 * 0x178 + 0xcc] = 1;
    }
    if (*(short *)(gstVifModule + param_1 * 0x178 + 0x120) == 0x100) {
      iVar2 = CarDV_VifDevInit(uVar3 + 3);
      if (iVar2 != 0) goto LAB_0002723e;
      gstVifModule[param_1 * 0x178 + 0x120] = 1;
    }
    iVar2 = 0;
  }
  return iVar2;
}

