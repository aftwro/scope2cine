
/* CarDV_VifDevInit(unsigned int) */

int CarDV_VifDevInit(uint param_1)

{
  undefined2 uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 local_a8;
  uint local_a4;
  int local_a0;
  int iStack_9c;
  int local_98;
  uint local_94;
  undefined4 local_90;
  undefined4 local_8c;
  undefined1 local_88;
  undefined2 local_87;
  undefined1 local_85;
  uint local_84;
  undefined4 local_80;
  undefined4 local_7c;
  int local_78;
  undefined4 local_74;
  undefined1 auStack_70 [36];
  uint local_4c;
  undefined4 local_48;
  int local_44;
  int local_40;
  int local_2c;
  
  uVar5 = param_1 >> 2;
  uVar6 = param_1 & 3;
  uVar8 = *(undefined4 *)(gstVifModule + (uVar5 * 0x5e + uVar6) * 4 + 8);
  uVar7 = *(undefined4 *)(gstVifModule + uVar5 * 0x178 + 4);
  memset(auStack_70,0,0x48);
  iVar2 = MI_SNR_GetPlaneInfo(uVar7,uVar8,auStack_70);
  if (iVar2 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_VifDevInit",0xf2,iVar2);
    return iVar2;
  }
  local_87 = 0;
  local_85 = 0;
  local_90 = local_48;
  local_94 = local_4c;
  local_98 = local_2c;
  if (local_44 < 0xc) {
    local_98 = local_40 * 0xc + local_44 + 0x14;
  }
  iVar3 = uVar5 * 0x178;
  iVar4 = uVar6 * 0x54 + iVar3;
  local_88 = gstVifModule[iVar4 + 0x2c];
  local_8c = *(undefined4 *)(gstVifModule + iVar4 + 0x28);
  iVar2 = MI_VIF_SetDevAttr(param_1,&local_98);
  if (iVar2 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_VifDevInit",0x104,iVar2);
    return iVar2;
  }
  iVar2 = MI_VIF_EnableDev(param_1);
  if (iVar2 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_VifDevInit",0x105,iVar2);
    return iVar2;
  }
  gstVifModule[iVar4 + 0x24] = 1;
  if ((gstVifModule[iVar3 + 0x174] == '\0') && (local_98 == 0 || local_98 - 0xeU < 3)) {
    gstVifModule[iVar3 + 0x174] = 1;
    iVar2 = MI_SNR_GetAnadecSrcAttr(uVar7,uVar8,&local_84);
    if (iVar2 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_VifDevInit",0x110,iVar2);
      return iVar2;
    }
    *(uint *)(gstVifModule + iVar4 + 100) = local_84;
    if (local_84 < 2) {
      *(undefined4 *)(g_camExisted + (uint)(byte)gstVifModule[iVar4 + 0x60] * 4) = 0;
      printf("Cam [%d] NOT Existed\n");
    }
  }
  iVar2 = uVar6 * 0x54 + uVar5 * 0x178;
  if (*(short *)(gstVifModule + iVar2 + 0x30) == 0x100) {
    local_80._2_2_ = (undefined2)((uint)local_48 >> 0x10);
    uVar1 = local_80._2_2_;
    if ((*(short *)(gstVifModule + iVar2 + 0x36) == 0) ||
       (*(short *)(gstVifModule + iVar2 + 0x38) == 0)) {
      local_80 = local_48;
      local_84 = local_4c;
      if (gstVifModule[uVar6 * 0x54 + uVar5 * 0x178 + 0x2c] != '\0') {
        local_80 = CONCAT22(uVar1,(ushort)local_48 >> 1);
      }
    }
    else {
      local_80 = CONCAT22(*(short *)(gstVifModule + iVar2 + 0x38),
                          *(short *)(gstVifModule + iVar2 + 0x36));
      local_84 = *(uint *)(gstVifModule + iVar2 + 0x32);
    }
    iVar2 = uVar6 * 0x54 + uVar5 * 0x178;
    if ((*(short *)(gstVifModule + iVar2 + 0x3a) == 0) ||
       (*(short *)(gstVifModule + iVar2 + 0x3c) == 0)) {
      local_7c = local_48;
      if (gstVifModule[uVar6 * 0x54 + uVar5 * 0x178 + 0x2c] != '\0') {
        local_7c = CONCAT22(uVar1,(ushort)local_48 >> 1);
      }
    }
    else {
      local_7c = CONCAT22(*(short *)(gstVifModule + iVar2 + 0x3c),
                          *(short *)(gstVifModule + iVar2 + 0x3a));
    }
    local_78 = *(int *)(gstVifModule + uVar6 * 0x54 + uVar5 * 0x178 + 0x40);
    if (0x53 < local_78) {
      if (local_44 < 0xc) {
        local_78 = local_40 * 0xc + local_44 + 0x14;
      }
      else {
        local_78 = local_2c;
      }
    }
    local_74 = 0;
    iVar2 = MI_VIF_SetOutputPortAttr(param_1);
    if (iVar2 != 0) goto LAB_00026f46;
    iVar2 = MI_VIF_EnableOutputPort(param_1,0);
    if (iVar2 != 0) goto LAB_00026f5c;
    iVar3 = uVar6 * 0x54 + uVar5 * 0x178;
    local_a8 = 6;
    gstVifModule[iVar3 + 0x30] = 1;
    local_a4 = param_1;
    local_a0 = iVar2;
    iStack_9c = iVar2;
    iVar2 = CarDV_GetRawEnable(gstVifModule[iVar3 + 0x60],(MI_SYS_ChnPort_s *)&local_a8);
    if (iVar2 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_VifDevInit",0x158,iVar2);
      return iVar2;
    }
  }
  iVar2 = uVar6 * 0x54 + uVar5 * 0x178;
  if (*(short *)(gstVifModule + iVar2 + 0x48) != 0x100) {
    return 0;
  }
  local_80._2_2_ = (undefined2)((uint)local_48 >> 0x10);
  uVar1 = local_80._2_2_;
  if ((*(short *)(gstVifModule + iVar2 + 0x4e) == 0) ||
     (*(short *)(gstVifModule + iVar2 + 0x50) == 0)) {
    local_80 = local_48;
    local_84 = local_4c;
    if (gstVifModule[uVar6 * 0x54 + uVar5 * 0x178 + 0x2c] != '\0') {
      local_80 = CONCAT22(uVar1,(ushort)local_48 >> 1);
    }
  }
  else {
    local_80 = CONCAT22(*(short *)(gstVifModule + iVar2 + 0x50),
                        *(short *)(gstVifModule + iVar2 + 0x4e));
    local_84 = *(uint *)(gstVifModule + iVar2 + 0x4a);
  }
  iVar2 = uVar6 * 0x54 + uVar5 * 0x178;
  if ((*(short *)(gstVifModule + iVar2 + 0x52) == 0) ||
     (*(short *)(gstVifModule + iVar2 + 0x54) == 0)) {
    local_7c = local_48;
    if (gstVifModule[uVar6 * 0x54 + uVar5 * 0x178 + 0x2c] != '\0') {
      local_7c = CONCAT22(uVar1,(ushort)local_48 >> 1);
    }
  }
  else {
    local_7c = CONCAT22(*(short *)(gstVifModule + iVar2 + 0x54),
                        *(short *)(gstVifModule + iVar2 + 0x52));
  }
  local_78 = *(int *)(gstVifModule + uVar6 * 0x54 + uVar5 * 0x178 + 0x58);
  if ((0x53 < local_78) && (local_78 = local_2c, local_44 < 0xc)) {
    local_78 = local_40 * 0xc + local_44 + 0x14;
  }
  local_74 = 0;
  iVar2 = MI_VIF_SetOutputPortAttr(param_1,1);
  if (iVar2 == 0) {
    iVar2 = MI_VIF_EnableOutputPort(param_1,1);
    if (iVar2 == 0) {
      gstVifModule[uVar6 * 0x54 + uVar5 * 0x178 + 0x48] = 1;
      return 0;
    }
LAB_00026f5c:
    printf("(%s %d)exec func fail, err:%x\n","CarDV_VifDevInit",0x14f,iVar2);
    return iVar2;
  }
LAB_00026f46:
  printf("(%s %d)exec func fail, err:%x\n","CarDV_VifDevInit",0x14e,iVar2);
  return iVar2;
}

