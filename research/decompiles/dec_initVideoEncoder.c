
/* MI_VideoEncoder::initVideoEncoder() */

int __thiscall MI_VideoEncoder::initVideoEncoder(MI_VideoEncoder *this)

{
  MI_VideoEncoder MVar1;
  uint uVar2;
  ushort uVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  void *pvVar7;
  uint extraout_r1;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  MI_SYS_ChnPort_s *pMVar12;
  undefined4 local_ec;
  undefined4 local_e8;
  undefined4 uStack_e4;
  undefined1 local_e0;
  undefined4 local_dc;
  uint local_d8;
  uint local_d4;
  undefined4 uStack_d0;
  uint local_cc [7];
  undefined4 local_b0;
  undefined4 local_ac;
  undefined4 local_a4;
  undefined4 uStack_a0;
  undefined4 local_9c;
  undefined4 local_98;
  undefined4 uStack_94;
  undefined4 local_74;
  uint local_70;
  uint local_6c;
  uint local_68;
  undefined4 local_64;
  uint local_60;
  uint local_5c;
  uint local_58;
  undefined4 local_54;
  undefined4 local_4c;
  uint local_48;
  uint uStack_44;
  uint local_40;
  uint uStack_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 uStack_30;
  
  memset(&local_74,0,0x4c);
  iVar4 = internalInit(this);
  if (iVar4 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","initVideoEncoder",0xa6,iVar4);
    return iVar4;
  }
  pMVar12 = (MI_SYS_ChnPort_s *)(this + 0x78);
  iVar4 = VideoInitResolution(pMVar12,(uint *)(this + 0x24),(uint *)(this + 0x28));
  if (iVar4 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","initVideoEncoder",0xa8,iVar4);
    return iVar4;
  }
  if ((((*(int *)(this + 0x78) == 0x22) && (*(int *)(this + 0x88) == 4)) && (0 < DAT_0008561c)) &&
     (0x1000 < *(uint *)(this + 0x24))) {
    uVar3 = __aeabi_uidiv(*(int *)(this + 0x28) << 0xc);
    iVar4 = VideoSetInputResolution(pMVar12,0x1000,(uint)uVar3);
    if (iVar4 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","initVideoEncoder",0xb3,iVar4);
      return iVar4;
    }
    *(uint *)(this + 0x28) = (uint)uVar3;
    *(undefined4 *)(this + 0x24) = 0x1000;
  }
  uVar9 = *(uint *)(this + 0x34);
  MVar1 = this[0x48];
  if (uVar9 == 0) {
    if ((byte)MVar1 < 2) {
      uVar9 = (uint)(*(int *)(this + 0x2c) * *(int *)(this + 0x30)) >> 1;
    }
    else {
      uVar9 = (uint)(*(int *)(this + 0x2c) * *(int *)(this + 0x30)) >> 2;
    }
    if (uVar9 < 0x25801) {
      uVar9 = uVar9 << 1;
    }
  }
  uVar11 = *(uint *)(this + 0x4c) & 0xffff;
  uVar8 = *(uint *)(this + 0x4c) >> 0x10;
  uVar6 = uVar11;
  uVar2 = uVar8;
  if (uVar8 == 0) {
    uVar8 = 1;
LAB_00038576:
    if (MVar1 == (MI_VideoEncoder)0x1) goto LAB_0003865a;
LAB_0003857a:
    if (MVar1 == (MI_VideoEncoder)0x2) {
      local_64 = CONCAT31(local_64._1_3_,1);
      local_60 = *(int *)(this + 0x24) + 7U & 0xfffffff8;
      local_5c = *(int *)(this + 0x28) + 1U & 0xfffffffe;
      local_70 = *(int *)(this + 0x2c) + 7U & 0xfffffff8;
      local_6c = *(int *)(this + 0x30) + 1U & 0xfffffffe;
      local_4c = 7;
      local_74 = 4;
      local_68 = uVar9;
      local_48 = uVar11;
      uStack_44 = uVar8;
      local_40 = *(uint *)(this + 100);
    }
    else if (MVar1 == (MI_VideoEncoder)0x0) {
      local_4c = 1;
      local_58 = *(int *)(this + 0x28) + 1U & 0xfffffffe;
      local_6c = *(int *)(this + 0x30) + 1U & 0xfffffffe;
      local_5c = *(int *)(this + 0x24) + 7U & 0xfffffff8;
      local_70 = *(int *)(this + 0x2c) + 7U & 0xfffffff8;
      local_38 = *(undefined4 *)(this + 0x38);
      local_64 = 2;
      local_54 = 2;
      local_60 = CONCAT31(local_60._1_3_,1);
      if (this[0x3c] != (MI_VideoEncoder)0x0) {
        uStack_30 = *(undefined4 *)(this + 0x5c);
        local_4c = 2;
        local_34 = *(undefined4 *)(this + 0x58);
      }
      local_74 = 2;
      local_68 = uVar9;
      local_48 = *(uint *)(this + 0x44);
      local_40 = uVar11;
      uStack_3c = uVar8;
    }
    else if (g_dbglevel < 6) {
      printf("\x1b[0;31m[ERR]:%s[%d]: \x1b[0m","initVideoEncoder",0x133);
      printf("error encoder type:%d\n",(uint)(byte)this[0x48]);
    }
  }
  else {
    do {
      uVar10 = uVar2;
      __aeabi_uidivmod(uVar6,uVar10);
      uVar6 = uVar10;
      uVar2 = extraout_r1;
    } while (extraout_r1 != 0);
    if (uVar10 == 1) goto LAB_00038576;
    uVar11 = __aeabi_uidiv(uVar11,uVar10);
    uVar8 = __aeabi_uidiv(uVar8,uVar10);
    if (MVar1 != (MI_VideoEncoder)0x1) goto LAB_0003857a;
LAB_0003865a:
    local_60 = CONCAT31(local_60._1_3_,MVar1);
    local_5c = *(int *)(this + 0x24) + 7U & 0xfffffff8;
    local_70 = *(int *)(this + 0x2c) + 7U & 0xfffffff8;
    local_58 = *(int *)(this + 0x28) + 1U & 0xfffffffe;
    local_6c = *(int *)(this + 0x30) + 1U & 0xfffffffe;
    local_48 = *(uint *)(this + 0x44);
    local_38 = *(undefined4 *)(this + 0x38);
    if (this[0x3c] == (MI_VideoEncoder)0x0) {
      local_4c = 8;
    }
    else {
      local_34 = *(undefined4 *)(this + 0x58);
      uStack_30 = *(undefined4 *)(this + 0x5c);
      local_4c = 9;
    }
    local_74 = 3;
    local_68 = uVar9;
    local_40 = uVar11;
    uStack_3c = uVar8;
  }
  iVar4 = *(int *)this;
  if ((&DAT_0008561c)[iVar4] == 0) {
    if (iVar4 == 0) {
      local_b0 = 0x1200;
      local_ac = 0xa20;
    }
    else {
      local_b0 = 0;
      local_ac = 0;
      if (iVar4 == 8) {
        local_b0 = 0x2000;
        local_ac = 0x2000;
      }
    }
    iVar4 = MI_VENC_CreateDev(iVar4,&local_b0);
    if (iVar4 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","initVideoEncoder",0x144,iVar4);
      return iVar4;
    }
    iVar4 = *(int *)this;
  }
  iVar4 = MI_VENC_CreateChn(iVar4,*(undefined4 *)(this + 4),&local_74);
  if (iVar4 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","initVideoEncoder",0x147,iVar4);
    return iVar4;
  }
  iVar4 = *(int *)this;
  MVar1 = this[0x48];
  (&DAT_0008561c)[iVar4] = (&DAT_0008561c)[iVar4] + 1;
  if ((byte)MVar1 < 2) {
    local_e0 = 0;
    local_e8 = 1;
    uStack_e4 = 0;
    iVar4 = MI_VENC_SetRefParam(iVar4,*(undefined4 *)(this + 4),&local_e8);
    if (iVar4 != 0) {
      printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x151,iVar4);
      return iVar4;
    }
    local_dc = 2;
    local_d4 = (uVar9 << 5) / 10;
    uStack_d0 = 2;
    local_d8 = (uVar9 * 0x18) / 5;
    iVar4 = MI_VENC_SetSuperFrameCfg(*(undefined4 *)this,*(undefined4 *)(this + 4),&local_dc);
    if (iVar4 != 0) {
      printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x158,iVar4);
      return iVar4;
    }
    local_ec = 2;
    iVar4 = MI_VENC_SetRcPriority(*(undefined4 *)this,*(undefined4 *)(this + 4),&local_ec);
    if (iVar4 != 0) {
      printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x15b,iVar4);
      return iVar4;
    }
    pvVar7 = memset(&local_b0,0,0x38);
    iVar4 = MI_VENC_GetRcParam(*(undefined4 *)this,*(undefined4 *)(this + 4),pvVar7);
    if (iVar4 != 0) {
      printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x15e,iVar4);
      return iVar4;
    }
    if ((this[0x48] == (MI_VideoEncoder)0x0) || (this[0x48] == (MI_VideoEncoder)0x1)) {
      local_98 = *(undefined4 *)(this + 0x50);
      local_9c = *(undefined4 *)(this + 0x60);
      if (this[0x3c] == (MI_VideoEncoder)0x0) {
        uStack_a0 = *(undefined4 *)(this + 0x5c);
        local_a4 = *(undefined4 *)(this + 0x58);
        uStack_94 = *(undefined4 *)(this + 0x54);
      }
      else {
        uStack_a0 = *(undefined4 *)(this + 0x40);
        local_a4 = local_9c;
        local_9c = local_98;
        local_98 = *(undefined4 *)(this + 0x54);
      }
    }
    iVar4 = MI_VENC_SetRcParam(*(undefined4 *)this,*(undefined4 *)(this + 4),pvVar7);
    if (iVar4 != 0) {
      printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x187,iVar4);
      return iVar4;
    }
    if (this[0x48] == (MI_VideoEncoder)0x0) {
      iVar4 = MI_VENC_GetH264Vui(*(undefined4 *)this,*(undefined4 *)(this + 4));
      if (iVar4 != 0) {
        printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x18d,iVar4);
        return iVar4;
      }
      local_cc[3] = 1000;
      iVar4 = __aeabi_uidiv(2000,uVar8);
      local_cc[4] = uVar11 * iVar4;
      iVar4 = MI_VENC_SetH264Vui(*(undefined4 *)this,*(undefined4 *)(this + 4),local_cc);
      if (iVar4 != 0) {
        printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",400,iVar4);
        return iVar4;
      }
    }
    else {
      if (this[0x48] != (MI_VideoEncoder)0x1) goto LAB_000385fe;
      iVar4 = MI_VENC_GetH265Vui(*(undefined4 *)this,*(undefined4 *)(this + 4));
      if (iVar4 != 0) {
        printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x195,iVar4);
        return iVar4;
      }
      local_cc[3] = 1000;
      iVar4 = __aeabi_uidiv(1000,uVar8);
      local_cc[4] = uVar11 * iVar4;
      iVar4 = MI_VENC_SetH265Vui(*(undefined4 *)this,*(undefined4 *)(this + 4),local_cc);
      if (iVar4 != 0) {
        printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x198,iVar4);
        return iVar4;
      }
    }
    uVar9 = (uint)(byte)this[0x48];
    if (uVar9 == 1) {
      local_cc[0] = 0;
      local_cc[1] = 3;
      local_cc[2] = 3;
      iVar4 = MI_VENC_SetH265Dblk(*(undefined4 *)this,*(undefined4 *)(this + 4));
      if (iVar4 != 0) {
        printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x1a2,iVar4);
        return iVar4;
      }
    }
    else if (uVar9 == 0) {
      local_cc[1] = 6;
      local_cc[2] = 6;
      local_cc[0] = uVar9;
      iVar4 = MI_VENC_SetH264Dblk(*(undefined4 *)this,*(undefined4 *)(this + 4),local_cc);
      if (iVar4 != 0) {
        printf("[%s %d]exec func fail, err:%x\n","initVideoEncoder",0x1aa,iVar4);
        return iVar4;
      }
    }
  }
LAB_000385fe:
  if ((*(int *)(this + 0x88) == 4) && (*(int *)(this + 0x78) == 0x22)) {
    iVar4 = VideoEnableInput(pMVar12,'\0');
    if (iVar4 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","initVideoEncoder",0x1b1,iVar4);
      return iVar4;
    }
  }
  else {
    bindVideoEncoder(this,'\x01');
  }
  iVar5 = MI_VENC_GetFd(*(undefined4 *)this,*(undefined4 *)(this + 4));
  *(int *)(this + 8) = iVar5;
  iVar4 = 0;
  if (iVar5 < 0) {
    if (g_dbglevel < 6) {
      iVar4 = -1;
      printf("\x1b[0;31m[ERR]:%s[%d]: \x1b[0m","initVideoEncoder",0x1bc);
      printf("venc[%d] get fd fail\n",*(undefined4 *)(this + 4));
    }
    else {
      iVar4 = -1;
    }
  }
  return iVar4;
}


