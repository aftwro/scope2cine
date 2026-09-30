
/* CarDV_SensorModuleInit(unsigned int) */

int CarDV_SensorModuleInit(uint param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  undefined1 *puVar8;
  uint local_dc;
  undefined1 auStack_d8 [52];
  ushort local_a4;
  ushort local_a2;
  ushort local_a0;
  ushort local_9e;
  ushort local_9c;
  ushort local_9a;
  uint local_98;
  undefined4 local_94;
  undefined1 auStack_90 [32];
  undefined1 auStack_70 [76];
  
  local_dc = 0;
  if (3 < param_1) {
    return -1;
  }
  iVar2 = param_1 * 0xc;
  if (*(short *)(gstSensorAttr + iVar2) == 1) {
    memset(&local_a4,0,0x34);
    memset(auStack_d8,0,0x34);
    memset(auStack_70,0,0x48);
    iVar1 = MI_SNR_SetPlaneMode(param_1,gstSensorAttr[iVar2 + 2]);
    if (iVar1 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_SensorModuleInit",0x3d,iVar1);
      return iVar1;
    }
    iVar1 = MI_SNR_QueryResCount(param_1,&local_dc);
    if (iVar1 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_SensorModuleInit",0x3e,iVar1);
      return iVar1;
    }
    uVar6 = (uint)(byte)gstSensorAttr[iVar2 + 3];
    if (local_dc <= uVar6) {
      uVar5 = *(uint *)(gstSensorAttr + iVar2 + 8);
      if (uVar5 < 1000) {
        if (uVar5 == 0) {
          uVar5 = 0x1e;
        }
      }
      else {
        uVar5 = uVar5 / 1000;
      }
      if (local_dc == 0) {
        uVar6 = 0;
      }
      else {
        uVar7 = 0;
        uVar6 = 0;
        uVar4 = 0;
        do {
          iVar2 = MI_SNR_GetRes(param_1,uVar4,&local_a4);
          if (iVar2 != 0) {
            printf("(%s %d)exec func fail, err:%x\n","CarDV_SensorModuleInit",0x4c);
            return iVar2;
          }
          printf("list index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n"
                 ,uVar4,(uint)local_a4,(uint)local_a2,(uint)local_a0,(uint)local_9e,(uint)local_9c,
                 (uint)local_9a,local_98,local_94,auStack_90);
          if ((uVar5 <= local_98) &&
             ((uint)*(ushort *)(gstSensorAttr + param_1 * 0xc + 4) <= (uint)local_9c)) {
            if (((uint)*(ushort *)(gstSensorAttr + param_1 * 0xc + 6) <= (uint)local_9a) &&
               (uVar3 = local_98 * (uint)local_9a * (uint)local_9c, uVar3 < uVar7 || uVar7 == 0)) {
              uVar6 = uVar4;
              uVar7 = uVar3;
            }
          }
          uVar4 = uVar4 + 1 & 0xff;
        } while (uVar4 < local_dc);
      }
    }
    iVar2 = MI_SNR_GetRes(param_1,uVar6,&local_a4);
    if (iVar2 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_SensorModuleInit",0x66,iVar2);
      return iVar2;
    }
    uVar5 = (uint)local_9c;
    uVar3 = (uint)local_9a;
    uVar7 = (uint)local_9e;
    uVar4 = (uint)local_a0;
    puVar8 = auStack_90;
    printf("select index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n"
           ,uVar6,(uint)local_a4,(uint)local_a2,uVar4,uVar7,uVar5,uVar3,local_98,local_94,puVar8);
    iVar2 = MI_SNR_SetRes(param_1,uVar6);
    if (iVar2 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_SensorModuleInit",0x6c,iVar2,uVar4,uVar7,uVar5
             ,uVar3,local_98,local_94,puVar8);
      return iVar2;
    }
    iVar2 = MI_SNR_Enable(param_1);
    if (iVar2 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_SensorModuleInit",0x6d,iVar2,uVar4,uVar7,uVar5
             ,uVar3,local_98,local_94,puVar8);
      return iVar2;
    }
    gstSensorAttr[param_1 * 0xc + 1] = 1;
    if (*(int *)(gstSensorAttr + param_1 * 0xc + 8) != 0) {
      iVar2 = MI_SNR_SetFps(param_1);
      if (iVar2 == 0) {
        return 0;
      }
      printf("(%s %d)exec func fail, err:%x\n","CarDV_SensorModuleInit",0x71,iVar2,uVar4,uVar7,uVar5
             ,uVar3,local_98,local_94,puVar8);
      return iVar2;
    }
  }
  return 0;
}

