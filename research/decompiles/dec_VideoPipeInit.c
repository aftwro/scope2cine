
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* CarDV_VideoPipeInit() */

int CarDV_VideoPipeInit(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  
  iVar1 = MI_SNR_InitDev(0);
  uVar3 = 0;
  if (iVar1 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_VideoPipeInit",0x35,iVar1);
    return iVar1;
  }
  do {
    uVar2 = uVar3 + 1;
    iVar1 = CarDV_SensorModuleInit(uVar3);
    if (iVar1 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_VideoPipeInit",0x38,iVar1);
      return iVar1;
    }
    uVar3 = uVar2;
  } while (uVar2 != 4);
  uVar3 = 0;
  do {
    uVar2 = uVar3 + 1;
    iVar1 = CarDV_VifModuleInit(uVar3);
    if (iVar1 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_VideoPipeInit",0x3d,iVar1);
      return iVar1;
    }
    uVar3 = uVar2;
  } while (uVar2 != 4);
  iVar1 = CarDV_IspModuleInit(0);
  if (iVar1 == 0) {
    uVar3 = 0;
    do {
      uVar2 = uVar3 + 1;
      iVar1 = CarDV_SclModuleInit(uVar3);
      if (iVar1 != 0) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_VideoPipeInit",0x5c,iVar1);
        return iVar1;
      }
      uVar3 = uVar2;
    } while (uVar2 != 4);
    iVar1 = CarDV_DispModuleInit();
    if (iVar1 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_VideoPipeInit",0x68,iVar1);
    }
  }
  else {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_VideoPipeInit",0x50,iVar1);
  }
  return iVar1;
}

