===== FUN_0003eaa0-0003eaa0

void FUN_0003eaa0(int param_1,int param_2,void *param_3,void *param_4)

{
  size_t sVar1;
  size_t __n;
  int iVar2;
  undefined4 local_1bc;
  undefined4 local_1b8 [4];
  undefined4 local_1a8;
  undefined1 auStack_1a4 [20];
  undefined2 local_190;
  undefined2 local_18e;
  undefined4 local_188;
  undefined1 auStack_130 [20];
  undefined1 local_11c;
  int local_110;
  int local_108;
  int local_100;
  ushort local_f8;
  ushort local_f6;
  void *local_f4;
  void *local_f0;
  uint local_d0;
  int local_cc;
  
  local_1b8[0] = carInfo._40_4_;
  local_1b8[1] = carInfo._44_4_;
  local_1b8[2] = carInfo._48_4_;
  local_1b8[3] = carInfo._52_4_;
  memset(auStack_130,0,0x108);
  memset(auStack_1a4,0,0x74);
  local_1a8 = 1;
  local_188 = 0xb;
  local_190 = (undefined2)param_1;
  local_18e = (undefined2)param_2;
  local_110 = MI_SYS_ChnInputPortGetBuf(local_1b8,&local_1a8,auStack_130,&local_1bc,0xffffffff);
  if (local_110 != 0) {
    return;
  }
  local_11c = 0;
  local_108 = local_110;
  local_100 = local_110;
  MI_SYS_GetCurPts(0,auStack_130);
  __n = param_2 * param_1;
  if (param_3 == (void *)0x0 || param_4 == (void *)0x0) {
    sVar1 = (size_t)local_f8;
    if (local_d0 == sVar1) {
      memset(local_f4,0,__n);
      memset(local_f0,0x80,(int)__n / 2);
    }
    else if (local_f6 != 0) {
      iVar2 = 0;
      while( true ) {
        memset((void *)(local_d0 * iVar2 + (int)local_f4),0,sVar1);
        iVar2 = iVar2 + 1;
        if ((int)(uint)local_f6 <= iVar2) break;
        sVar1 = (size_t)local_f8;
      }
      iVar2 = 0;
      if (local_f6 >> 1 != 0) {
        do {
          memset((void *)(local_cc * iVar2 + (int)local_f0),0x80,(uint)local_f8);
          iVar2 = iVar2 + 1;
        } while (iVar2 < (int)(uint)(local_f6 >> 1));
        MI_SYS_ChnInputPortPutBuf(local_1bc,auStack_130,0);
        return;
      }
    }
  }
  else {
    sVar1 = (size_t)local_f8;
    if (local_d0 == sVar1) {
      memcpy(local_f4,param_3,__n);
      memcpy(local_f0,param_4,(int)__n / 2);
    }
    else if (local_f6 != 0) {
      iVar2 = 0;
      while( true ) {
        memcpy((void *)(local_d0 * iVar2 + (int)local_f4),(void *)(iVar2 * sVar1 + (int)param_3),
               sVar1);
        iVar2 = iVar2 + 1;
        if ((int)(uint)local_f6 <= iVar2) break;
        sVar1 = (size_t)local_f8;
      }
      iVar2 = 0;
      if (local_f6 >> 1 != 0) {
        do {
          memcpy((void *)(local_cc * iVar2 + (int)local_f0),
                 (void *)(iVar2 * (uint)local_f8 + (int)param_4),(uint)local_f8);
          iVar2 = iVar2 + 1;
        } while (iVar2 < (int)(uint)(local_f6 >> 1));
      }
    }
  }
  MI_SYS_ChnInputPortPutBuf(local_1bc,auStack_130,0);
  return;
}


===== FUN_0003ec40-0003ec40

void FUN_0003ec40(int param_1,int param_2,void *param_3)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  size_t __n;
  undefined4 local_1b4;
  undefined4 local_1b0 [4];
  undefined4 local_1a0;
  undefined1 auStack_19c [20];
  undefined2 local_188;
  undefined2 local_186;
  undefined1 auStack_128 [20];
  undefined1 local_114;
  int local_108;
  int local_100;
  int local_f8;
  ushort local_f0;
  ushort local_ee;
  void *local_ec;
  int local_c8;
  
  local_1b0[1] = carInfo._44_4_;
  local_1b0[0] = carInfo._40_4_;
  local_1b0[2] = carInfo._48_4_;
  local_1b0[3] = carInfo._52_4_;
  memset(auStack_128,0,0x108);
  memset(auStack_19c,0,0x74);
  local_1a0 = 1;
  local_188 = (undefined2)param_1;
  local_186 = (undefined2)param_2;
  local_108 = MI_SYS_ChnInputPortGetBuf(local_1b0,&local_1a0,auStack_128,&local_1b4,0xffffffff);
  if (local_108 == 0) {
    local_114 = 0;
    local_100 = local_108;
    local_f8 = local_108;
    MI_SYS_GetCurPts(0,auStack_128);
    __n = param_2 * param_1 * 2;
    if (param_3 == (void *)0x0) {
      uVar2 = (uint)local_f0;
      if (local_c8 == uVar2 * 2) {
        memset(local_ec,0,__n);
      }
      else {
        iVar4 = 0;
        if (local_ee != 0) {
          while( true ) {
            memset((void *)(local_c8 * iVar4 + (int)local_ec),0,uVar2 << 1);
            iVar4 = iVar4 + 1;
            if ((int)(uint)local_ee <= iVar4) break;
            uVar2 = (uint)local_f0;
          }
        }
      }
    }
    else {
      uVar2 = (uint)local_f0;
      if (local_c8 == uVar2 * 2) {
        memcpy(local_ec,param_3,__n);
      }
      else {
        iVar4 = 0;
        if (local_ee != 0) {
          while( true ) {
            iVar3 = iVar4 * uVar2;
            iVar1 = local_c8 * iVar4;
            iVar4 = iVar4 + 1;
            memcpy((void *)(iVar1 + (int)local_ec),(void *)((int)param_3 + iVar3 * 2),uVar2 << 1);
            if ((int)(uint)local_ee <= iVar4) break;
            uVar2 = (uint)local_f0;
          }
        }
      }
    }
    MI_SYS_ChnInputPortPutBuf(local_1b4,auStack_128,0);
    return;
  }
  return;
}


