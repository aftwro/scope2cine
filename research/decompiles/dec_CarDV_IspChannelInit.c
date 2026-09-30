
/* CarDV_IspChannelInit(unsigned int, unsigned int) */

int CarDV_IspChannelInit(uint param_1,uint param_2)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 local_88;
  uint local_84;
  uint local_80;
  uint local_7c;
  undefined4 local_78 [21];
  
  iVar4 = param_2 * 0x130 + param_1 * 0x130c;
  memset(local_78,0,0x50);
  switch(*(undefined4 *)(gstIspModule + iVar4 + 0x9c)) {
  case 0:
    local_78[0] = 1;
    break;
  case 1:
    local_78[0] = 2;
    break;
  case 2:
    local_78[0] = 4;
    iVar2 = MI_ISP_CreateChannel(param_1,param_2,local_78);
    goto joined_r0x0002f8b6;
  case 3:
    local_78[0] = 8;
    break;
  default:
    local_78[0] = 1;
    printf("[%s]%d snrPad%d fail \n","_TransMISnrPadToMIIspBindSensorId",0x45);
  }
  iVar2 = MI_ISP_CreateChannel(param_1,param_2,local_78);
joined_r0x0002f8b6:
  if (iVar2 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_IspChannelInit",0x1af,iVar2);
    return iVar2;
  }
  iVar2 = param_2 * 0x130 + param_1 * 0x130c;
  if (((*(short *)(gstIspModule + iVar2 + 0x14) != 0) &&
      (*(short *)(gstIspModule + iVar2 + 0x16) != 0)) &&
     (iVar2 = MI_ISP_SetInputPortCrop(param_1,param_2,iVar4 + 0x7bdec), iVar2 != 0)) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_IspChannelInit",0x1b3,iVar2);
    return iVar2;
  }
  iVar2 = MI_ISP_SetChnParam(param_1,param_2,iVar4 + 0x7be68);
  if (iVar2 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_IspChannelInit",0x1b6,iVar2);
    return iVar2;
  }
  iVar2 = MI_ISP_StartChannel(param_1,param_2);
  if (iVar2 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_IspChannelInit",0x1b7,iVar2);
    return iVar2;
  }
  iVar3 = param_2 * 0x130 + param_1 * 0x130c;
  gstIspModule[iVar3 + 7] = 1;
  iVar2 = access(gstIspModule + iVar4 + 0xa0,0);
  if (iVar2 == 0) {
    gstIspModule[iVar3 + 0x120] = 0;
    pthread_create((pthread_t *)(gstIspModule + iVar4 + 0x124),(pthread_attr_t *)0x0,
                   (__start_routine *)0x2f129,gstIspModule + iVar4 + 4);
  }
  else {
    printf("ISP [%s] Not Exist\n",gstIspModule + iVar4 + 0xa0);
  }
  iVar2 = param_2 * 0x130 + param_1 * 0x130c;
  if (*(short *)(gstIspModule + iVar2 + 0x38) == 1) {
    iVar3 = MI_ISP_EnableOutputPort(param_1,param_2,0);
    if (iVar3 != 0) goto LAB_0002f998;
    gstIspModule[iVar2 + 0x39] = 1;
  }
  iVar2 = param_2 * 0x130 + param_1 * 0x130c;
  uVar1 = *(ushort *)(gstIspModule + iVar2 + 0x54);
  if (uVar1 == 1) {
    local_88 = *(undefined4 *)(gstIspModule + iVar4 + 100);
    local_84 = *(uint *)(gstIspModule + iVar4 + 0x68);
    local_80 = *(uint *)(gstIspModule + iVar2 + 0x5c);
    iVar4 = MI_ISP_SetOutputPortParam(param_1,param_2,1,&local_88);
    if (iVar4 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_IspChannelInit",0x1cf,iVar4);
      return iVar4;
    }
    iVar3 = MI_ISP_EnableOutputPort(param_1,param_2,1);
    if (iVar3 != 0) goto LAB_0002f998;
    local_88 = 0x21;
    local_84 = param_1;
    local_80 = param_2;
    local_7c = (uint)uVar1;
    iVar4 = MI_SYS_SetChnOutputPortDepth
                      (0,&local_88,*(undefined2 *)(gstIspModule + iVar2 + 0x6e),
                       *(undefined2 *)(gstIspModule + iVar2 + 0x6c));
    if (iVar4 != 0) goto LAB_0002f9e4;
    gstIspModule[iVar2 + 0x55] = (char)uVar1;
  }
  iVar2 = param_2 * 0x130 + param_1 * 0x130c;
  if (*(short *)(gstIspModule + iVar2 + 0x70) != 1) {
    return 0;
  }
  iVar3 = MI_ISP_EnableOutputPort(param_1,param_2,2);
  if (iVar3 == 0) {
    local_88 = 0x21;
    local_7c = 2;
    local_84 = param_1;
    local_80 = param_2;
    iVar4 = MI_SYS_SetChnOutputPortDepth
                      (0,&local_88,*(undefined2 *)(gstIspModule + iVar2 + 0x8a),
                       *(undefined2 *)(gstIspModule + iVar2 + 0x88));
    if (iVar4 == 0) {
      gstIspModule[iVar2 + 0x71] = 1;
      return 0;
    }
LAB_0002f9e4:
    printf("(%s %d)exec func fail, err:%x\n","CarDV_IspChannelInit",0x1db,iVar4);
    return iVar4;
  }
LAB_0002f998:
  printf("(%s %d)exec func fail, err:%x\n","CarDV_IspChannelInit",0x1d2);
  return iVar3;
}

