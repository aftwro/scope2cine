
undefined4 isp_process_cmd(undefined4 param_1,uint *param_2)

{
  byte bVar1;
  ushort uVar2;
  ushort uVar3;
  byte bVar4;
  undefined4 uVar5;
  undefined2 uVar6;
  uint uVar7;
  uint uVar8;
  undefined2 *puVar9;
  uint uVar10;
  int iVar11;
  uint uVar12;
  uint in_fpscr;
  double dVar13;
  double dVar14;
  double __y;
  double __x;
  uint local_66d0;
  uint local_66cc;
  uint local_66c8;
  uint local_66c4;
  uint local_66c0;
  uint local_66bc;
  uint uStack_66b8;
  undefined4 local_66b4;
  undefined4 local_66b0;
  undefined4 local_66ac;
  uint local_66a8;
  undefined4 local_66a4;
  undefined4 local_66a0;
  undefined4 local_669c;
  undefined4 local_6698;
  undefined4 local_6694;
  uint auStack_6690 [16];
  uint local_6650;
  uint local_664c;
  uint local_6648;
  uint local_6644;
  int local_6640;
  undefined4 uStack_663c;
  uint local_6608;
  byte local_64c8;
  undefined1 auStack_6048 [43];
  undefined1 local_601d;
  undefined1 auStack_5448 [245];
  byte abStack_5353 [19721];
  undefined2 uStack_64a;
  undefined2 local_648 [256];
  undefined2 local_448 [256];
  undefined2 local_248 [258];
  
  uVar7 = 0x78000;
  switch(param_1) {
  case 0x403:
    bVar1 = (byte)*param_2;
    MI_ISP_IQ_GetSaturation(isp_dev,isp_chn,&local_6650);
    printf("IQ_SetSaturation u8SatAllStr [0,127] Cur=%d\n",(uint)local_64c8);
    bVar4 = bVar1;
    if (bVar1 == 0xff) {
      bVar4 = local_64c8;
    }
    local_64c8 = bVar4;
    local_664c = (uint)(bVar1 != 0xff);
    local_6650 = 1;
    MI_ISP_IQ_SetSaturation(isp_dev,isp_chn,&local_6650);
    printf("set Saturation value = %u\n",(uint)local_64c8);
    return 0;
  case 0x404:
    uVar7 = *param_2;
    MI_ISP_IQ_GetSharpness(isp_dev,isp_chn,&local_6650);
    printf("MI_ISP_IQ_SHARPNESS_TYPE_t u8SclByY [0,255] Cur=%d\n",(uint)abStack_5353[0]);
    local_6650 = 1;
    local_664c = 1;
    memcpy(auStack_5448,&local_6648,0x120);
    memset(abStack_5353,(uint)(byte)uVar7,9);
    printf("Set Sharpness Value = %d\n",(int)(char)(byte)uVar7);
    MI_ISP_IQ_SetSharpness(isp_dev,isp_chn,&local_6650);
    return 0;
  case 0x405:
    uVar8 = (uint)(char)(byte)*param_2;
    MI_ISP_IQ_GetLightness(isp_dev,isp_chn,&local_6650);
    printf("IQ_SetLightness [0,100] Cur=%d\n",local_6608);
    if (100 < uVar8) {
      uVar7 = 0;
    }
    local_6650 = 1;
    local_664c = 1;
    if (100 < uVar8) {
      uVar8 = uVar7;
    }
    local_6608 = uVar8;
    MI_ISP_IQ_SetLightness(isp_dev,isp_chn,&local_6650);
    printf("Lightness:%d\n",uVar8);
    return 0;
  default:
    return 0;
  case 0x407:
    MI_ISP_AE_GetEVComp(isp_dev,isp_chn);
    local_6650 = *param_2;
    printf("EXPOSURE: %+d\n");
    MI_ISP_AE_SetEVComp(isp_dev,isp_chn,&local_6650);
    return 0;
  case 0x40a:
    memset(&local_6650,0,0x20);
    MI_ISP_AE_GetExposureLimit(isp_dev,isp_chn,&local_6650);
    if ((byte)*param_2 != 0) {
      local_664c = 250000;
      MI_ISP_AE_SetExposureLimit(isp_dev,isp_chn,&local_6650);
      return 0;
    }
    local_664c = 0x8234;
    MI_ISP_AE_SetExposureLimit(isp_dev,isp_chn,&local_6650);
    return 0;
  case 0x40b:
    auStack_6690[0] = 0;
    switch((byte)*param_2) {
    case 1:
      auStack_6690[0] = 4;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,auStack_6690);
      MI_ISP_AE_GetManualExpo(isp_dev,isp_chn,&local_6650);
      local_6644 = 0x411a;
      MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_6650);
      return 0;
    case 2:
      auStack_6690[0] = 4;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,auStack_6690);
      MI_ISP_AE_GetManualExpo(isp_dev,isp_chn,&local_6650);
      local_6644 = 0x208d;
      MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_6650);
      return 0;
    case 3:
      auStack_6690[0] = 4;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,auStack_6690);
      MI_ISP_AE_GetManualExpo(isp_dev,isp_chn,&local_6650);
      local_6644 = 0x1046;
      MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_6650);
      return 0;
    case 4:
      auStack_6690[0] = 4;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,auStack_6690);
      MI_ISP_AE_GetManualExpo(isp_dev,isp_chn,&local_6650);
      local_6644 = 2000;
      MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_6650);
      return 0;
    case 5:
      auStack_6690[0] = 4;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,auStack_6690);
      MI_ISP_AE_GetManualExpo(isp_dev,isp_chn,&local_6650);
      local_6644 = 1000;
      MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_6650);
      return 0;
    default:
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,auStack_6690);
      return 0;
    }
  case 0x412:
    uVar7 = *param_2;
    MI_ISP_IQ_GetContrast(isp_dev,isp_chn,&local_6650);
    printf("IQ_SetContrast:[0,100] Cur=%d\n",local_6608);
    local_6650 = 1;
    local_664c = 1;
    local_6608 = (int)(char)(byte)uVar7;
    MI_ISP_IQ_SetContrast(isp_dev,isp_chn,&local_6650);
    printf("set Contrast value = %d\n",(int)(char)(byte)uVar7);
    return 0;
  case 0x413:
    iVar11 = (int)(char)(byte)*param_2;
    MI_ISP_IQ_GetBrightness(isp_dev,isp_chn,&local_6650);
    printf("IQ_SetBrightness:[0,100] Cur=%d\n",local_6608);
    printf("nValue = %u\n",iVar11);
    local_6650 = 1;
    local_664c = 1;
    local_6608 = iVar11;
    MI_ISP_IQ_SetBrightness(isp_dev,isp_chn,&local_6650);
    printf("set Brightness value = %d\n",iVar11);
    return 0;
  case 0x415:
    goto switchD_0003158e_caseD_415;
  case 0x41a:
    local_66c4 = param_2[3];
    local_66d0 = *param_2;
    local_66c8 = param_2[2];
    local_66cc = param_2[1];
    MI_ISP_AWB_GetAttr(isp_dev,isp_chn,&local_6650);
    uVar8 = local_66d0;
    local_6650 = 0;
    printf("WB MODE: %d\n",local_66d0);
    uVar7 = local_6644 >> 0x10;
    printf("WB R = %d,G = (%d,%d),B = %d\n",local_6648 & 0xffff,local_6648 >> 0x10,
           local_6644 & 0xffff,uVar7);
    if (uVar8 == 0xffffffff) {
      local_66c0 = 0;
      local_66a8 = 0;
      local_66bc = 0;
      uStack_66b8 = 0;
      local_66b4 = 0;
      local_66b0 = 0;
      local_66ac = 0;
      local_66a4 = 0;
      local_66a0 = 0;
      local_669c = 0;
      local_6698 = 0;
      local_6694 = 0;
      if (local_664c != 0) {
        local_664c = 0;
        MI_ISP_AWB_SetAttr(isp_dev,isp_chn,&local_6650);
      }
      uVar8 = 0;
      do {
        MI_ISP_AWB_QueryInfo(isp_dev,isp_chn,&local_66c0);
        local_6648 = local_66a4;
        if (local_66bc == uVar8) {
          uVar3 = local_66a0._2_2_;
          uVar8 = (uint)local_66a0._2_2_;
          if (uVar8 == uStack_66b8 >> 0x10) goto code_r0x00031d14;
        }
        local_66a8 = local_66c0;
        local_66a4 = local_66bc;
        local_66a0 = uStack_66b8;
        local_669c = local_66b4;
        local_6698 = local_66b0;
        local_6694 = local_66ac;
        usleep(200000);
        uVar8 = local_66a4;
      } while( true );
    }
    if (uVar8 == 1) {
      uVar6 = (undefined2)
              ((uint)((int)((ulonglong)(local_66c8 << 0xd) * 0x80808081 >> 0x20) << 9) >> 0x10);
      local_6648 = CONCAT22(uVar6,(short)((local_66cc << 0xd) / 0xff));
      local_6644 = CONCAT22((short)((local_66c4 << 0xd) / 0xff),uVar6);
      local_664c = uVar8;
    }
    else {
      local_664c = 0;
    }
    goto LAB_00031a06;
  case 0x41b:
    iVar11 = (int)(char)(byte)*param_2;
    local_66a8 = 0;
    local_66a4 = 0;
    local_66a0 = 0;
    local_669c = 0;
    local_66d0 = 0;
    printf("CMD_ISP_SET_ISO value = %d\n",iVar11);
    switch(iVar11) {
    case 0:
      local_66d0 = 0;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,&local_66d0);
      local_6650 = 0;
      MI_ISP_AE_SetState(isp_dev,isp_chn,&local_6650);
      return 0;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      local_66d0 = 4;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,&local_66d0);
      MI_ISP_AE_GetManualExpo(isp_dev,isp_chn,&local_66a8);
      local_66a4 = 0x400 << (iVar11 - 1U & 0xff);
      local_66a0 = 0x400;
      MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_66a8);
      return 0;
    case -3:
      local_66d0 = 4;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,&local_66d0);
      MI_ISP_AE_GetManualExpo(isp_dev,isp_chn,&local_66a8);
      local_66a8 = GKU_VS20_EXPFNX10_GET();
      local_66a0 = GKU_VS20_ISPGIAN_GET();
      local_66a4 = GKU_VS20_SENSORGIAN_GET();
      local_669c = GKU_VS20_EXPUS_GET();
      printf("u32ISPGain = %d,u32SensorGain = %d\n",local_66a0,local_66a4);
      MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_66a8);
      return 0;
    case -2:
      memset(&local_6650,0,0x23c);
      local_66d0 = 0;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,&local_66d0);
      local_66c0 = 0;
      MI_ISP_AE_SetState(isp_dev,isp_chn,&local_66c0);
      usleep(500000);
      break;
    case -1:
      local_6650 = 1;
      MI_ISP_AE_SetState(isp_dev,isp_chn,&local_6650);
      return 0;
    default:
      local_66d0 = 4;
      MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,&local_66d0);
      MI_ISP_AE_GetManualExpo(isp_dev,isp_chn,&local_66a8);
      local_66a0 = 0x400;
      local_66a4 = iVar11;
      MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_66a8);
      return 0;
    }
    while ((MI_ISP_AE_QueryExposureInfo(isp_dev,isp_chn,&local_6650), local_6640 != local_66a0 ||
           (local_6644 != local_66a4))) {
      local_66a8 = local_6648;
      local_66a4 = local_6644;
      local_66a0 = local_6640;
      local_669c = uStack_663c;
      usleep(200000);
    }
    printf("ispGain = %d , senseotGain = %d\n");
    snprintf((char *)auStack_6690,0x40,"%d,%d,%d,%d",local_66a8,local_66a0,local_66a4,local_669c);
    IPC_MsgToUI_SendMsg(0xd,auStack_6690,0x40);
    GKU_VS20_EXP_SET(local_66a8,local_66a0,local_66a4,local_669c);
    local_66d0 = 4;
    MI_ISP_AE_SetExpoMode(isp_dev,isp_chn,&local_66d0);
    MI_ISP_AE_SetManualExpo(isp_dev,isp_chn,&local_66a8);
    return 0;
  case 0x41c:
    uVar7 = *param_2;
    memset(&local_6650,0,0x6608);
    MI_ISP_IQ_GetRGBGamma(isp_dev,isp_chn,&local_6650);
    dVar14 = (double)VectorSignedToFloat((int)(char)(byte)uVar7,(byte)(in_fpscr >> 0x16) & 3);
    __y = 1.06742 - dVar14 / 100.0;
    dVar14 = pow(255.0,__y);
    local_6650 = 1;
    local_664c = 1;
    if ((char)(byte)uVar7 == 0x38) {
      memcpy(local_248,RGBGamma_VS20,0x200);
      memcpy(local_448,RGBGamma_VS20,0x200);
      memcpy(local_648,RGBGamma_VS20,0x200);
    }
    else {
      puVar9 = &uStack_64a;
      __x = 0.0;
      dVar14 = (double)VectorSignedToFloat((uint)(0.0 < 1024.0 / dVar14) *
                                           (int)(longlong)(1024.0 / dVar14) & 0xffff,
                                           (byte)(in_fpscr >> 0x16) & 3);
      do {
        dVar13 = pow(__x,__y);
        __x = __x + 1.0;
        uVar7 = (uint)(0.0 < dVar13 * dVar14) * (int)(longlong)(dVar13 * dVar14);
        uVar6 = (undefined2)uVar7;
        if (0x3fe < (uVar7 & 0xffff)) {
          uVar6 = 0x3ff;
        }
        puVar9[0x201] = uVar6;
        puVar9[0x101] = uVar6;
        puVar9 = puVar9 + 1;
        *puVar9 = uVar6;
      } while (local_648 + 0xff != puVar9);
    }
    MI_ISP_IQ_SetRGBGamma(isp_dev,isp_chn,&local_6650);
    return 0;
  case 0x41f:
    MI_ISP_AE_GetWinWgtType(isp_dev,isp_chn,&local_6650);
    local_6650 = (uint)(char)(byte)*param_2;
    if ((local_6650 != 0) && (local_6650 != 1)) {
      local_6650 = 2;
    }
    MI_ISP_AE_SetWinWgtType(isp_dev,isp_chn,&local_6650);
    return 0;
  case 0x420:
    uVar7 = *param_2;
    MI_ISP_IQ_GetWDR(isp_dev,isp_chn,&local_6650);
    printf("CMD_ISP_SET_HDR value = %d\n",uVar7);
    memcpy(auStack_6048,&local_6648,0x60);
    local_601d = (undefined1)uVar7;
    local_6650 = 1;
    local_664c = 1;
    MI_ISP_IQ_SetWDR(isp_dev,isp_chn,&local_6650);
    puts("set HDR");
    return 0;
  case 0x421:
    uVar7 = *param_2;
    if (19999 < uVar7) {
      uVar7 = 20000;
    }
    if (uVar7 < 1000) {
      uVar7 = 1000;
    }
    MI_ISP_AWB_GetAttr(isp_dev,isp_chn,&local_6650);
    if ((local_664c != 2) || (local_6650 != 0)) {
      local_6650 = 0;
      local_664c = 2;
      MI_ISP_AWB_SetAttr(isp_dev,isp_chn,&local_6650);
    }
    MI_ISP_AWB_GetCTMwbAttr(isp_dev,isp_chn);
    printf("get CTM wb = %d\n",auStack_6690[0]);
    auStack_6690[0] = uVar7;
    MI_ISP_AWB_SetCTMwbAttr(isp_dev,isp_chn,auStack_6690);
    printf("CMD_ISP_SET_CTMWB : %d\n",auStack_6690[0]);
    return 0;
  }
code_r0x00031d14:
  uVar2 = local_66a4._2_2_;
  uVar10 = (uint)local_66a4._2_2_;
  uVar12 = local_66a4 & 0xffff;
  printf("r,g,b = %d,%d,%d\n",uVar12,uVar10,uVar8,uVar7);
  local_6644 = CONCAT22(uVar3,uVar2);
  local_664c = 1;
  snprintf((char *)auStack_6690,0x40,"%d,%d,%d",(int)(uVar12 * 0xff) >> 0xd,
           (int)(uVar10 * 0xff) >> 0xd,(int)(uVar8 * 0xff) >> 0xd);
  IPC_MsgToUI_SendMsg(0xc,auStack_6690,0x40);
LAB_00031a06:
  MI_ISP_AWB_SetAttr(isp_dev,isp_chn,&local_6650);
  return 0;
switchD_0003158e_caseD_415:
  MI_ISP_AE_GetFlicker(isp_dev,isp_chn,&local_6650);
  if ((byte)*param_2 == 0x32) {
    local_6650 = 2;
  }
  else {
    if ((byte)*param_2 == 0x3c) {
      local_6650 = 1;
      uVar5 = 0x3c;
      goto LAB_00031612;
    }
    local_6650 = 0;
  }
  uVar5 = 0x32;
LAB_00031612:
  printf("FLICKER: %dHZ\n",uVar5);
  MI_ISP_AE_SetFlicker(isp_dev,isp_chn,&local_6650);
  return 0;
}


