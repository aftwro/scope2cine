
/* CarDV_IspModuleInit(unsigned int) */

int CarDV_IspModuleInit(uint param_1)

{
  short sVar1;
  int iVar2;
  int iVar3;
  undefined4 local_24 [2];
  
  iVar3 = param_1 * 0x130c;
  if ((*(uint *)(gstIspModule + iVar3) & 0xffff00) == 0x100) {
    local_24[0] = 1;
    iVar2 = MI_ISP_CreateDevice(param_1,local_24);
    if (iVar2 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_IspModuleInit",0x212,iVar2);
      return iVar2;
    }
    gstIspModule[iVar3 + 2] = 1;
    sVar1 = *(short *)(gstIspModule + param_1 * 0x130c + 6);
  }
  else {
    sVar1 = *(short *)(gstIspModule + param_1 * 0x130c + 6);
  }
  if ((sVar1 == 1) && (iVar2 = CarDV_IspChannelInit(param_1,0), iVar2 != 0)) goto LAB_0002fdba;
  if (*(short *)(gstIspModule + param_1 * 0x130c + 0x136) == 1) {
    iVar2 = CarDV_IspChannelInit(param_1,1);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x266) != 1) goto LAB_0002fc64;
LAB_0002fdf2:
    iVar2 = CarDV_IspChannelInit(param_1,2);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x396) != 1) goto LAB_0002fc76;
LAB_0002fe12:
    iVar2 = CarDV_IspChannelInit(param_1,3);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x4c6) != 1) goto LAB_0002fc88;
LAB_0002fe32:
    iVar2 = CarDV_IspChannelInit(param_1,4);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x5f6) != 1) goto LAB_0002fc9a;
LAB_0002fe52:
    iVar2 = CarDV_IspChannelInit(param_1,5);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x726) != 1) goto LAB_0002fcac;
LAB_0002fe72:
    iVar2 = CarDV_IspChannelInit(param_1,6);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x856) != 1) goto LAB_0002fcbe;
LAB_0002fe92:
    iVar2 = CarDV_IspChannelInit(param_1,7);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x986) != 1) goto LAB_0002fcd0;
LAB_0002feb2:
    iVar2 = CarDV_IspChannelInit(param_1,8);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0xab6) != 1) goto LAB_0002fce2;
LAB_0002fed4:
    iVar2 = CarDV_IspChannelInit(param_1,9);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0xbe6) != 1) goto LAB_0002fcf4;
LAB_0002fef6:
    iVar2 = CarDV_IspChannelInit(param_1,10);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0xd16) != 1) goto LAB_0002fd06;
LAB_0002ff18:
    iVar2 = CarDV_IspChannelInit(param_1,0xb);
    if (iVar2 != 0) goto LAB_0002fdba;
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0xe46) != 1) goto LAB_0002fd18;
LAB_0002ff3a:
    iVar2 = CarDV_IspChannelInit(param_1,0xc);
    if (iVar2 != 0) goto LAB_0002fdba;
    sVar1 = *(short *)(gstIspModule + param_1 * 0x130c + 0xf76);
  }
  else {
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x266) == 1) goto LAB_0002fdf2;
LAB_0002fc64:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x396) == 1) goto LAB_0002fe12;
LAB_0002fc76:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x4c6) == 1) goto LAB_0002fe32;
LAB_0002fc88:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x5f6) == 1) goto LAB_0002fe52;
LAB_0002fc9a:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x726) == 1) goto LAB_0002fe72;
LAB_0002fcac:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x856) == 1) goto LAB_0002fe92;
LAB_0002fcbe:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0x986) == 1) goto LAB_0002feb2;
LAB_0002fcd0:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0xab6) == 1) goto LAB_0002fed4;
LAB_0002fce2:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0xbe6) == 1) goto LAB_0002fef6;
LAB_0002fcf4:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0xd16) == 1) goto LAB_0002ff18;
LAB_0002fd06:
    if (*(short *)(gstIspModule + param_1 * 0x130c + 0xe46) == 1) goto LAB_0002ff3a;
LAB_0002fd18:
    sVar1 = *(short *)(gstIspModule + param_1 * 0x130c + 0xf76);
  }
  if ((((sVar1 != 1) || (iVar2 = CarDV_IspChannelInit(param_1,0xd), iVar2 == 0)) &&
      ((*(short *)(gstIspModule + param_1 * 0x130c + 0x10a6) != 1 ||
       (iVar2 = CarDV_IspChannelInit(param_1,0xe), iVar2 == 0)))) &&
     ((*(short *)(gstIspModule + param_1 * 0x130c + 0x11d6) != 1 ||
      (iVar2 = CarDV_IspChannelInit(param_1,0xf), iVar2 == 0)))) {
    gstIspModule[param_1 * 0x130c + 0x1304] = 0;
    pthread_create((pthread_t *)(gstIspModule + iVar3 + 0x1308),(pthread_attr_t *)0x0,
                   (__start_routine *)0x2f175,gstIspModule + iVar3);
    return 0;
  }
LAB_0002fdba:
  printf("(%s %d)exec func fail, err:%x\n","CarDV_IspModuleInit",0x21b,iVar2);
  return iVar2;
}

