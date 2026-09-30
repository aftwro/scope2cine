
int MI_ISP_CreateChannel(undefined4 param_1,undefined4 param_2,void *param_3)

{
  int iVar1;
  int *piVar2;
  char *pcVar3;
  undefined2 local_78 [2];
  undefined4 local_74;
  undefined4 *local_70;
  int iStack_6c;
  undefined4 local_68;
  undefined4 uStack_64;
  undefined1 auStack_60 [80];
  
  local_68 = param_1;
  uStack_64 = param_2;
  memcpy(auStack_60,param_3,0x50);
  local_70 = &local_68;
  local_78[0] = 0;
  iStack_6c = (int)local_70 >> 0x1f;
  local_74 = 0x58;
  iVar1 = FUN_00017938();
  iVar1 = ioctl(iVar1,0x40586902,local_78);
  if (iVar1 != -1) {
    MI_ISP_EnableUserspace3A(param_1,param_2);
    return iVar1;
  }
  piVar2 = __errno_location();
  pcVar3 = strerror(*piVar2);
  printf("[%s] failed to ioctl 0x%08lx!(%s)\n",&DAT_00020a90,0x40586902,pcVar3);
  MI_ISP_EnableUserspace3A(param_1,param_2);
  return -1;
}


