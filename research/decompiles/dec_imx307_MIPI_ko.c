//==== pCus_poweroff @ 00010028

void pCus_poweroff(int param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 0x44);
  (**(code **)(iVar1 + 0xc))(param_2,*(undefined4 *)(param_1 + 0x58));
  (**(code **)(iVar1 + 0x10))(param_2,0,*(undefined4 *)(param_1 + 0x9fc));
  (**(code **)(iVar1 + 0x20))(param_2,0xffffffff);
  if (*(int *)(param_1 + 0x9e0) == 1) {
    (**(code **)(iVar1 + 0x34))(param_2,1,0);
  }
  *(undefined4 *)(param_1 + 0x84) = 0;
  return;
}


//==== imx307_SetPatternMode @ 00010066

int imx307_SetPatternMode(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  undefined2 *puVar1;
  int iVar2;
  ushort uVar3;
  
  puVar1 = DAT_00010094;
  if (param_2 != 1) {
    uVar3 = DAT_00010094[1];
  }
  else {
    uVar3 = 0x21;
  }
  iVar2 = *(int *)(param_1 + 0x40);
  if (param_2 != 1) {
    uVar3 = uVar3 & 0xfe;
  }
  DAT_00010094[1] = uVar3;
  iVar2 = (**(code **)(iVar2 + 0x14))(iVar2,param_1 + 0x30,*puVar1,puVar1[1],param_4);
  return -(uint)(iVar2 != 0);
}


//==== pCus_GetVideoResNum @ 00010098

undefined4 pCus_GetVideoResNum(int param_1,undefined4 *param_2)

{
  *param_2 = *(undefined4 *)(param_1 + 0x90);
  return 0;
}


//==== pCus_GetVideoRes @ 000100a2

undefined4 pCus_GetVideoRes(int param_1,uint param_2,int *param_3)

{
  undefined4 uVar1;
  
  if (param_2 < *(uint *)(param_1 + 0x90)) {
    uVar1 = 0;
    *param_3 = param_2 * 0x4c + param_1 + 0x98;
  }
  else {
    uVar1 = 0xffffffff;
  }
  return uVar1;
}


//==== pCus_GetCurVideoRes @ 000100be

undefined4 pCus_GetCurVideoRes(int param_1,uint *param_2,int *param_3)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar3 = *(uint *)(param_1 + 0x90);
  uVar2 = *(uint *)(param_1 + 0x94);
  *param_2 = uVar2;
  if (uVar2 < uVar3) {
    uVar1 = 0;
    *param_3 = uVar2 * 0x4c + param_1 + 0x98;
  }
  else {
    uVar1 = 0xffffffff;
  }
  return uVar1;
}


//==== pCus_SetVideoRes @ 000100de

undefined4 pCus_SetVideoRes(int param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = DAT_00010110;
  if (param_2 < *(uint *)(param_1 + 0x90)) {
    if (param_2 == 0) {
      *(undefined4 *)(param_1 + 0x94) = 0;
      uVar2 = DAT_00010114;
      if (*(int *)(iVar1 + 4) == 4) {
        uVar2 = DAT_00010118;
      }
      *(undefined4 *)(param_1 + 0xa14) = uVar2;
      *(undefined4 *)(*(int *)(param_1 + 0x28) + 0x38) = 0x465;
    }
    return 0;
  }
  return 0xffffffff;
}


//==== pCus_SetVideoRes_HDR_DOL @ 0001011c

undefined4 pCus_SetVideoRes_HDR_DOL(int param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = DAT_00010154;
  if (param_2 < *(uint *)(param_1 + 0x90)) {
    if (param_2 == 0) {
      *(undefined4 *)(param_1 + 0x94) = 0;
      uVar2 = DAT_00010158;
      if ((*(int *)(iVar1 + 8) == 2) || (uVar2 = DAT_0001015c, *(int *)(iVar1 + 8) == 4)) {
        *(undefined4 *)(param_1 + 0xa14) = uVar2;
        *(undefined4 *)(*(int *)(param_1 + 0x28) + 0x38) = 0x486;
      }
    }
    return 0;
  }
  return 0xffffffff;
}


//==== pCus_GetOrien @ 00010160

undefined4 pCus_GetOrien(int param_1,undefined4 *param_2)

{
  *param_2 = *(undefined4 *)(param_1 + 0x84);
  return 0;
}


//==== pCus_SetOrien @ 0001016a

undefined4 pCus_SetOrien(int param_1,undefined4 param_2)

{
  *(undefined4 *)(param_1 + 0x84) = param_2;
  *(undefined1 *)(*(int *)(param_1 + 0x28) + 0xd2) = 1;
  return 0;
}


//==== FUN_0001017a @ 0001017a

undefined4 FUN_0001017a(int param_1,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = *(int *)(param_1 + 0x40);
  if (param_2 == 2) {
    uVar2 = 1;
  }
  else if (param_2 == 3) {
    uVar2 = 3;
  }
  else {
    if (param_2 != 1) {
      (**(code **)(iVar1 + 0x14))(iVar1,param_1 + 0x30,0x3007,0);
      *(undefined4 *)(param_1 + 0x84) = 0;
      return 0;
    }
    uVar2 = 2;
  }
  (**(code **)(iVar1 + 0x14))(iVar1,param_1 + 0x30,0x3007,uVar2);
  *(int *)(param_1 + 0x84) = param_2;
  return 0;
}


//==== pCus_GetFPS @ 000101bc

void pCus_GetFPS(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  
  iVar3 = *(int *)(param_1 + 0x28);
  iVar1 = DAT_000101fc;
  if (*(uint *)(iVar3 + 0x58) < 3000) {
    iVar1 = 0x465;
  }
  uVar2 = __aeabi_uidiv(*(int *)(*(int *)(param_1 + 0x94) * 0x4c + param_1 + 0xa0) * iVar1,
                        (uint)*(ushort *)(iVar3 + 0x94) << 8 |
                        (uint)*(ushort *)(iVar3 + 0x90) << 0x10 | (uint)*(ushort *)(iVar3 + 0x98));
  *(undefined4 *)(iVar3 + 0x5c) = uVar2;
  return;
}


//==== pCus_GetFPS_HDR_DOL_SEF1 @ 00010200

void pCus_GetFPS_HDR_DOL_SEF1(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  
  iVar3 = *(int *)(param_1 + 0x28);
  iVar1 = DAT_00010240;
  if (*(uint *)(iVar3 + 0x58) < 3000) {
    iVar1 = 0x486;
  }
  uVar2 = __aeabi_uidiv(*(int *)(*(int *)(param_1 + 0x94) * 0x4c + param_1 + 0xa0) * iVar1,
                        (uint)*(ushort *)(iVar3 + 0x94) << 8 |
                        (uint)*(ushort *)(iVar3 + 0x90) << 0x10 | (uint)*(ushort *)(iVar3 + 0x98));
  *(undefined4 *)(iVar3 + 0x5c) = uVar2;
  return;
}


//==== pCus_AEStatusNotify @ 00010244

undefined4 pCus_AEStatusNotify(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if (param_2 == 1) {
    iVar1 = *(int *)(param_1 + 0x28);
    iVar3 = *(int *)(param_1 + 0x44);
    if ((*(char *)(iVar1 + 0x8c) != '\0') || (*(char *)(iVar1 + 0xd2) != '\0')) {
      iVar2 = param_1 + 0x30;
      (**(code **)(*(int *)(param_1 + 0x40) + 0x14))(*(int *)(param_1 + 0x40),iVar2,0x3001,1);
      if (*(char *)(iVar1 + 0x8c) != '\0') {
        (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))
                  (*(int *)(param_1 + 0x40),iVar2,iVar1 + 0xa6,3);
        (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))
                  (*(int *)(param_1 + 0x40),iVar2,iVar1 + 0x9a,3);
        (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))
                  (*(int *)(param_1 + 0x40),iVar2,iVar1 + 0x8e,3);
        *(undefined1 *)(iVar1 + 0x8c) = 0;
      }
      if (*(char *)(iVar1 + 0xd2) != '\0') {
        FUN_0001017a(param_1,*(undefined4 *)(param_1 + 0x84));
        *(undefined1 *)(iVar1 + 0xd2) = 0;
      }
      (**(code **)(*(int *)(param_1 + 0x40) + 0x14))(*(int *)(param_1 + 0x40),iVar2,0x3001,0);
    }
    if (*(int *)(iVar1 + 0x88) != 0) {
      (**(code **)(iVar3 + 0x3c))(*(undefined4 *)(param_1 + 0x88),*(undefined4 *)(iVar1 + 0x58));
      *(undefined4 *)(iVar1 + 0x88) = 0;
    }
  }
  return 0;
}


//==== pCus_AEStatusNotifyHDR_DOL_SEF1 @ 000102f2

undefined4 pCus_AEStatusNotifyHDR_DOL_SEF1(void)

{
  return 0;
}


//==== pCus_GetAEUSecs @ 000102f6

undefined4 pCus_GetAEUSecs(int param_1,uint *param_2)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 0x28);
  *param_2 = (uint)((ulonglong)
                    (((*(ushort *)(iVar1 + 0xa8) & 0xff) << 0x10 |
                      (uint)(ushort)(*(short *)(iVar1 + 0xac) << 8) | (uint)*(byte *)(iVar1 + 0xb0))
                    * 0x73bd) * (ulonglong)DAT_00010328 >> 0x26);
  return 0;
}


//==== pCus_SetAEUSecs @ 0001032c

undefined4 pCus_SetAEUSecs(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  
  iVar2 = *(int *)(param_1 + 0x28);
  *(int *)(iVar2 + 100) = param_2;
  if ((uint)(param_2 * 1000) < 0x73bd) {
    uVar3 = 1;
  }
  else {
    uVar3 = (uint)((ulonglong)(uint)(param_2 * 1000) * (ulonglong)DAT_00010390 >> 0x2e);
  }
  iVar1 = *(int *)(iVar2 + 0x38);
  *(uint *)(iVar2 + 0x60) = uVar3;
  if (iVar1 - 2U < uVar3) {
    iVar1 = uVar3 + 2;
  }
  iVar4 = ~uVar3 + iVar1;
  *(ushort *)(iVar2 + 0xa8) = (ushort)((uint)(iVar4 * 0x4000) >> 0x1e);
  *(ushort *)(iVar2 + 0xac) = (ushort)(byte)((uint)iVar4 >> 8);
  *(ushort *)(iVar2 + 0xb0) = (ushort)iVar4 & 0xff;
  *(ushort *)(iVar2 + 0x90) = (ushort)((uint)(iVar1 << 0xe) >> 0x1e);
  *(ushort *)(iVar2 + 0x94) = (ushort)(byte)((uint)iVar1 >> 8);
  *(ushort *)(iVar2 + 0x98) = (ushort)iVar1 & 0xff;
  *(undefined1 *)(iVar2 + 0x8c) = 1;
  return 0;
}


//==== pCus_SetFPS @ 00010394

undefined4 pCus_SetFPS(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = *(int *)(param_1 + 0x28);
  if (param_2 - 3 < 0x1c) {
    if ((*(int *)(param_1 + 0x9fc) == 4) && (0x1c < param_2)) {
      param_2 = 0x1d;
    }
    uVar2 = param_2 * 500;
    *(uint *)(iVar4 + 0x58) = param_2;
    param_2 = param_2 * 1000;
  }
  else {
    if (27000 < param_2 - 3000) {
      return 0xffffffff;
    }
    if ((*(int *)(param_1 + 0x9fc) == 4) && (0x71a2 < param_2)) {
      param_2 = 0x71a3;
    }
    uVar2 = param_2 >> 1;
    *(uint *)(iVar4 + 0x58) = param_2;
  }
  iVar3 = DAT_0001041c;
  if (*(int *)(param_1 + 0x9fc) != 4) {
    iVar3 = DAT_00010420;
  }
  uVar1 = __aeabi_uidiv(iVar3 + uVar2,param_2,*(int *)(param_1 + 0x9fc),uVar2,param_4);
  *(undefined1 *)(iVar4 + 0x8c) = 1;
  *(undefined4 *)(iVar4 + 0x38) = uVar1;
  iVar3 = *(int *)(iVar4 + 0x38);
  if (iVar3 - 2U < *(uint *)(iVar4 + 0x60)) {
    iVar3 = *(uint *)(iVar4 + 0x60) + 8;
  }
  *(int *)(iVar4 + 0x38) = iVar3;
  pCus_SetAEUSecs(param_1,*(undefined4 *)(iVar4 + 100));
  return 0;
}


//==== pCus_SetAEUSecsHDR_DOL_SEF1 @ 00010424

void pCus_SetAEUSecsHDR_DOL_SEF1(int param_1,int param_2)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  
  iVar6 = *(int *)(param_1 + 0x28);
  uVar7 = param_2 * 1000 + 0x1c1c;
  *(int *)(iVar6 + 0x68) = param_2;
  uVar3 = *(uint *)(iVar6 + 0x58);
  if (uVar3 < 0xbb9) {
    if (0x19 < uVar3) {
      if (DAT_00010510 <= uVar7) goto LAB_0001044e;
      uVar2 = 0x87b4;
      goto LAB_000104e6;
    }
LAB_00010504:
    uVar5 = 0xd3;
  }
  else {
    if (uVar3 < 0x61a9) goto LAB_00010504;
    uVar2 = DAT_00010518;
    if (DAT_00010510 <= uVar7) {
LAB_0001044e:
      *(undefined4 *)(iVar6 + 0x80) = 0xd3;
      *(undefined4 *)(iVar6 + 0x38) = 0x56d;
      goto LAB_0001045a;
    }
LAB_000104e6:
    uVar2 = __aeabi_uidiv(uVar2);
    uVar5 = 0x65;
    *(undefined4 *)(iVar6 + 0x38) = uVar2;
  }
  *(undefined4 *)(iVar6 + 0x80) = uVar5;
LAB_0001045a:
  uVar8 = *(uint *)(iVar6 + 0x38);
  uVar3 = *(uint *)(iVar6 + 0x80);
  if ((uVar8 * 2 - uVar3) - 3 < *(uint *)(iVar6 + 0x60)) {
    uVar8 = *(uint *)(iVar6 + 0x60) + uVar3 + 4 >> 1;
  }
  if (uVar7 < 0x3838) {
    uVar7 = 1;
  }
  else {
    uVar7 = (uint)((ulonglong)(uVar7 >> 3) * (ulonglong)DAT_00010514 >> 0x27);
  }
  uVar1 = uVar3 - 3;
  if (uVar1 <= uVar7) {
    uVar7 = uVar1;
  }
  if (uVar7 < uVar1) {
    if (*(int *)(iVar6 + 0x84) - 2U < uVar3) {
      iVar4 = 0;
    }
    else {
      iVar4 = (uVar3 - 1) - uVar7;
    }
  }
  else {
    iVar4 = 2;
  }
  *(ushort *)(iVar6 + 0x90) = (ushort)((uVar8 << 0xe) >> 0x1e);
  *(undefined2 *)(iVar6 + 0xc0) = 0;
  *(ushort *)(iVar6 + 0x94) = (ushort)(byte)(uVar8 >> 8);
  *(undefined2 *)(iVar6 + 0xc4) = 0;
  *(ushort *)(iVar6 + 0x98) = (ushort)uVar8 & 0xff;
  *(ushort *)(iVar6 + 0xa8) = (ushort)((uint)(iVar4 << 0xe) >> 0x1e);
  *(ushort *)(iVar6 + 0xac) = (ushort)(byte)((uint)iVar4 >> 8);
  *(ushort *)(iVar6 + 0xb0) = (ushort)iVar4 & 0xff;
  *(ushort *)(iVar6 + 200) = (ushort)uVar3 & 0xff;
  return;
}


//==== pCus_SetFPS_HDR_DOL_SEF1 @ 0001051c

undefined4 pCus_SetFPS_HDR_DOL_SEF1(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  
  iVar4 = *(int *)(param_1 + 0x28);
  *(uint *)(iVar4 + 0x58) = param_2;
  if (param_2 - 3 < 0x1c) {
    if ((*(int *)(param_1 + 0x9fc) == 4) && (0x1c < param_2)) {
      param_2 = 0x1d;
    }
    uVar3 = param_2 * 500;
    *(uint *)(iVar4 + 0x58) = param_2;
    param_2 = param_2 * 1000;
  }
  else {
    if (27000 < param_2 - 3000) {
      return 0xffffffff;
    }
    if ((*(int *)(param_1 + 0x9fc) == 4) && (0x71a2 < param_2)) {
      param_2 = 0x71a3;
    }
    uVar3 = param_2 >> 1;
    *(uint *)(iVar4 + 0x58) = param_2;
  }
  if (*(int *)(param_1 + 0x9fc) == 4) {
    iVar1 = uVar3 + 0x2020752;
  }
  else {
    iVar1 = DAT_000105c4 + uVar3;
  }
  uVar2 = __aeabi_uidiv(iVar1,param_2,*(int *)(param_1 + 0x9fc),uVar3,param_4);
  *(undefined1 *)(iVar4 + 0x8c) = 1;
  *(undefined4 *)(iVar4 + 0x38) = uVar2;
  uVar3 = *(uint *)(iVar4 + 0x38);
  if ((uVar3 * 2 - *(int *)(iVar4 + 0x80)) - 3 < *(uint *)(iVar4 + 0x60)) {
    uVar3 = *(uint *)(iVar4 + 0x60) + *(int *)(iVar4 + 0x80) + 4 >> 1;
  }
  *(uint *)(iVar4 + 0x38) = uVar3;
  *(uint *)(iVar4 + 0x88) = (uint)(*(int *)(iVar4 + 0x68) != 0);
  pCus_SetAEUSecsHDR_DOL_SEF1(param_1);
  return 0;
}


//==== pCus_GetAEGain @ 000105c8

undefined4 pCus_GetAEGain(int param_1,undefined4 *param_2)

{
  *param_2 = *(undefined4 *)(*(int *)(param_1 + 0x28) + 0x50);
  return 0;
}


//==== pCus_GetAEGainHDR_DOL_SEF1 @ 000105d2

undefined4 pCus_GetAEGainHDR_DOL_SEF1(int param_1,undefined4 *param_2)

{
  *param_2 = *(undefined4 *)(*(int *)(param_1 + 0x28) + 0x70);
  return 0;
}


//==== pCus_GetAEMinMaxUSecs @ 000105dc

undefined4 pCus_GetAEMinMaxUSecs(undefined4 param_1,undefined4 *param_2,undefined4 *param_3)

{
  *param_2 = 1;
  *param_3 = DAT_000105e8;
  return 0;
}


//==== pCus_GetAEMinMaxGain @ 000105ec

undefined4 pCus_GetAEMinMaxGain(undefined4 param_1,undefined4 *param_2,undefined4 *param_3)

{
  *param_2 = 0x400;
  *param_3 = DAT_000105fc;
  return 0;
}


//==== IMX307_GetShutterInfo @ 00010600

undefined4 IMX307_GetShutterInfo(undefined4 param_1,int param_2)

{
  undefined4 uVar1;
  
  uVar1 = DAT_00010610;
  *(undefined4 *)(param_2 + 0xc) = 0x73bd;
  *(undefined4 *)(param_2 + 4) = uVar1;
  *(undefined4 *)(param_2 + 8) = 0x73bd;
  return 0;
}


//==== pCus_setCaliData_gain_linearity @ 00010614

undefined4 pCus_setCaliData_gain_linearity(undefined4 param_1,int param_2,int param_3)

{
  undefined4 *puVar1;
  int iVar2;
  
  puVar1 = DAT_0001063c;
  for (iVar2 = 0; iVar2 != param_3; iVar2 = iVar2 + 1) {
    *puVar1 = *(undefined4 *)(param_2 + iVar2 * 8);
    puVar1[1] = *(undefined4 *)(param_2 + 4 + iVar2 * 8);
    puVar1 = puVar1 + 2;
  }
  return 0;
}


//==== IMX307_GetShutterInfo_hdr_dol_sef1 @ 00010640

undefined4 IMX307_GetShutterInfo_hdr_dol_sef1(undefined4 param_1,int param_2)

{
  *(undefined4 *)(param_2 + 4) = DAT_00010654;
  *(undefined4 *)(param_2 + 8) = 0x7070;
  *(undefined4 *)(param_2 + 0xc) = 0x3838;
  return 0;
}


//==== pCus_GetSensorID_isp_calibration @ 00010658

void pCus_GetSensorID_isp_calibration(undefined4 param_1,undefined4 *param_2)

{
  *param_2 = 0;
  return;
}


//==== pCus_GetVideoRes_isp_calibration @ 0001065e

undefined4 pCus_GetVideoRes_isp_calibration(int param_1,int param_2,int *param_3)

{
  *param_3 = param_2 * 0x4c + param_1 + 0x98;
  return 0;
}


//==== pCus_SetVideoRes_isp_calibration @ 0001066c

void pCus_SetVideoRes_isp_calibration(int param_1)

{
  *(undefined4 *)(param_1 + 0x94) = 0;
  return;
}


//==== pCus_GetOrien_isp_calibration @ 00010676

undefined4 pCus_GetOrien_isp_calibration(void)

{
  return 0;
}


//==== pCus_GetAEMinMaxUSecs_isp_calibration @ 0001067a

undefined4
pCus_GetAEMinMaxUSecs_isp_calibration(undefined4 param_1,undefined4 *param_2,undefined4 *param_3)

{
  *param_2 = 1;
  *param_3 = 0x8235;
  return 0;
}


//==== pCus_GetAEMinMaxGain_isp_calibration @ 00010688

undefined4
pCus_GetAEMinMaxGain_isp_calibration(undefined4 param_1,undefined4 *param_2,undefined4 *param_3)

{
  *param_2 = 0x400;
  *param_3 = 0x2000;
  return 0;
}


//==== pCus_setCaliData_gain_linearity_isp_calibration @ 00010698

undefined4 pCus_setCaliData_gain_linearity_isp_calibration(void)

{
  return 0;
}


//==== IMX307_GetShutterInfo_isp_calibration @ 0001069c

undefined4 IMX307_GetShutterInfo_isp_calibration(undefined4 param_1,int param_2)

{
  undefined4 uVar1;
  
  uVar1 = DAT_000106ac;
  *(undefined4 *)(param_2 + 0xc) = 0x73bd;
  *(undefined4 *)(param_2 + 4) = uVar1;
  *(undefined4 *)(param_2 + 8) = 0x73bd;
  return 0;
}


//==== pCus_GetFPS_hdr_dol_lef @ 000106b0

void pCus_GetFPS_hdr_dol_lef(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  
  iVar3 = *(int *)(param_1 + 0x28);
  iVar1 = DAT_00010240;
  if (*(uint *)(iVar3 + 0x58) < 3000) {
    iVar1 = 0x486;
  }
  uVar2 = __aeabi_uidiv(*(int *)(*(int *)(param_1 + 0x94) * 0x4c + param_1 + 0xa0) * iVar1,
                        (uint)*(ushort *)(iVar3 + 0x94) << 8 |
                        (uint)*(ushort *)(iVar3 + 0x90) << 0x10 | (uint)*(ushort *)(iVar3 + 0x98));
  *(undefined4 *)(iVar3 + 0x5c) = uVar2;
  return;
}


//==== pCus_AEStatusNotifyHDR_DOL_LEF @ 000106b2

undefined4 pCus_AEStatusNotifyHDR_DOL_LEF(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if (param_2 == 1) {
    iVar1 = *(int *)(param_1 + 0x28);
    iVar3 = *(int *)(param_1 + 0x44);
    if ((*(char *)(iVar1 + 0x8c) != '\0') || (*(char *)(iVar1 + 0xd2) != '\0')) {
      iVar2 = param_1 + 0x30;
      (**(code **)(*(int *)(param_1 + 0x40) + 0x14))(*(int *)(param_1 + 0x40),iVar2,0x3001,1);
      (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))(*(int *)(param_1 + 0x40),iVar2,iVar1 + 0xb2,3);
      (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))(*(int *)(param_1 + 0x40),iVar2,iVar1 + 0x8e,3);
      (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))(*(int *)(param_1 + 0x40),iVar2,iVar1 + 0xbe,3);
      (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))(*(int *)(param_1 + 0x40),iVar2,iVar1 + 0xa6,3);
      (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))(*(int *)(param_1 + 0x40),iVar2,iVar1 + 0xca,1);
      (**(code **)(*(int *)(param_1 + 0x40) + 0x1c))(*(int *)(param_1 + 0x40),iVar2,iVar1 + 0xce,1);
      *(undefined1 *)(iVar1 + 0x8c) = 0;
      if (*(char *)(iVar1 + 0xd2) != '\0') {
        FUN_0001017a(param_1,*(undefined4 *)(param_1 + 0x84));
        *(undefined1 *)(iVar1 + 0xd2) = 0;
      }
      (**(code **)(*(int *)(param_1 + 0x40) + 0x14))(*(int *)(param_1 + 0x40),iVar2,0x3001,0);
    }
    if (*(int *)(iVar1 + 0x88) != 0) {
      (**(code **)(iVar3 + 0x3c))(*(undefined4 *)(param_1 + 0x88),*(undefined4 *)(iVar1 + 0x58));
      *(undefined4 *)(iVar1 + 0x88) = 0;
    }
  }
  return 0;
}


//==== pCus_SetAEUSecsHDR_DOL_LEF @ 0001077c

undefined4 pCus_SetAEUSecsHDR_DOL_LEF(int param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  
  iVar5 = *(int *)(param_1 + 0x28);
  *(int *)(iVar5 + 100) = param_2;
  uVar2 = param_2 * 1000 + 0x1c1c;
  uVar1 = *(uint *)(iVar5 + 0x38);
  uVar3 = (uint)((ulonglong)(uVar2 >> 3) * (ulonglong)DAT_00010810 >> 0x27);
  *(uint *)(iVar5 + 0x60) = uVar3;
  if ((uVar1 * 2 - *(int *)(iVar5 + 0x80)) - 3 < uVar3) {
    uVar1 = uVar3 + *(int *)(iVar5 + 0x80) + 4 >> 1;
  }
  if (uVar2 < 0x7070) {
    *(undefined4 *)(iVar5 + 0x60) = 1;
  }
  uVar2 = uVar1 * 2 - 0x68;
  if (uVar2 < *(uint *)(iVar5 + 0x60)) {
    *(uint *)(iVar5 + 0x60) = uVar2;
  }
  iVar4 = ~*(uint *)(iVar5 + 0x60) + uVar1 * 2;
  *(int *)(iVar5 + 0x84) = iVar4;
  *(ushort *)(iVar5 + 0xb4) = (ushort)((uint)(iVar4 * 0x4000) >> 0x1e);
  *(ushort *)(iVar5 + 0xb8) = (ushort)(byte)((uint)iVar4 >> 8);
  *(ushort *)(iVar5 + 0xbc) = (ushort)iVar4 & 0xff;
  *(ushort *)(iVar5 + 0x90) = (ushort)((uVar1 << 0xe) >> 0x1e);
  *(ushort *)(iVar5 + 0x94) = (ushort)(byte)(uVar1 >> 8);
  *(ushort *)(iVar5 + 0x98) = (ushort)uVar1 & 0xff;
  *(undefined1 *)(iVar5 + 0x8c) = 1;
  return 0;
}


//==== pCus_SetFPS_hdr_dol_lef @ 00010814

undefined4 pCus_SetFPS_hdr_dol_lef(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  
  iVar4 = *(int *)(param_1 + 0x28);
  if (param_2 - 3 < 0x1c) {
    if ((*(int *)(param_1 + 0x9fc) == 4) && (0x1c < param_2)) {
      param_2 = 0x1d;
    }
    uVar3 = param_2 * 500;
    *(uint *)(iVar4 + 0x58) = param_2;
    param_2 = param_2 * 1000;
  }
  else {
    if (27000 < param_2 - 3000) {
      return 0xffffffff;
    }
    if ((*(int *)(param_1 + 0x9fc) == 4) && (0x71a2 < param_2)) {
      param_2 = 0x71a3;
    }
    uVar3 = param_2 >> 1;
    *(uint *)(iVar4 + 0x58) = param_2;
  }
  if (*(int *)(param_1 + 0x9fc) == 4) {
    iVar1 = uVar3 + 0x2020752;
  }
  else {
    iVar1 = DAT_000108b8 + uVar3;
  }
  uVar2 = __aeabi_uidiv(iVar1,param_2,*(int *)(param_1 + 0x9fc),uVar3,param_4);
  *(undefined1 *)(iVar4 + 0x8c) = 1;
  *(undefined4 *)(iVar4 + 0x38) = uVar2;
  uVar3 = *(uint *)(iVar4 + 0x38);
  if ((uVar3 * 2 - *(int *)(iVar4 + 0x80)) - 3 < *(uint *)(iVar4 + 0x60)) {
    uVar3 = *(uint *)(iVar4 + 0x60) + *(int *)(iVar4 + 0x80) + 4 >> 1;
  }
  *(uint *)(iVar4 + 0x38) = uVar3;
  *(uint *)(iVar4 + 0x88) = (uint)(*(int *)(iVar4 + 100) != 0);
  pCus_SetAEUSecsHDR_DOL_LEF(param_1);
  return 0;
}


//==== pCus_GetAEGain_hdr_dol_lef @ 000108bc

undefined4 pCus_GetAEGain_hdr_dol_lef(int param_1,undefined4 *param_2)

{
  *param_2 = *(undefined4 *)(*(int *)(param_1 + 0x28) + 0x6c);
  return 0;
}


//==== pCus_GetAEMinMaxGain_hdr_dol_lef @ 000108c6

undefined4 pCus_GetAEMinMaxGain_hdr_dol_lef(int param_1,undefined4 *param_2,undefined4 *param_3)

{
  *param_2 = *(undefined4 *)(param_1 + 0xa00);
  *param_3 = DAT_000108d4;
  return 0;
}


//==== IMX307_GetShutterInfo_hdr_dol_lef @ 000108d8

undefined4 IMX307_GetShutterInfo_hdr_dol_lef(undefined4 param_1,int param_2)

{
  undefined4 uVar1;
  
  uVar1 = DAT_000108f0;
  *(undefined4 *)(param_2 + 4) = DAT_000108ec;
  *(undefined4 *)(param_2 + 8) = uVar1;
  *(undefined4 *)(param_2 + 0xc) = 0x3838;
  return 0;
}


//==== cus_camsensor_release_handle @ 000108f4

undefined4 cus_camsensor_release_handle(void)

{
  return 0;
}


//==== FUN_000108f8 @ 000108f8

undefined4 FUN_000108f8(int param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 uStack_c;
  
  uStack_c = param_2;
  iVar1 = (**(code **)(*(int *)(param_1 + 0x40) + 0x18))
                    (*(int *)(param_1 + 0x40),param_1 + 0x30,0x31dc,(int)&uStack_c + 2,param_1);
  if (iVar1 == 0) {
    if ((uStack_c._2_2_ & 6) == 4) {
      return 0;
    }
    CamOsPrintf(DAT_00010928);
  }
  return 0xffffffff;
}


//==== pCus_SetAEGain_cal @ 0001092c

undefined4 pCus_SetAEGain_cal(int param_1,uint param_2)

{
  undefined2 uVar1;
  uint uVar2;
  int iVar3;
  bool bVar4;
  longlong lVar5;
  longlong lVar6;
  
  uVar2 = DAT_00010984;
  iVar3 = *(int *)(param_1 + 0x28);
  bVar4 = param_2 <= DAT_00010984;
  *(uint *)(iVar3 + 0x50) = param_2;
  if (bVar4) {
    uVar2 = param_2;
  }
  if (uVar2 < 0x400) {
    uVar2 = 0x400;
  }
  lVar5 = intlog10(uVar2);
  lVar6 = intlog10(0x400);
  lVar5 = (lVar5 - lVar6) * 200;
  uVar2 = (uint)((ulonglong)lVar5 >> 0x20);
  uVar1 = __aeabi_uldivmod((uint)lVar5 >> 0x18 | uVar2 * 0x100,uVar2 >> 0x18,3,0);
  *(undefined1 *)(iVar3 + 0x8c) = 1;
  *(undefined2 *)(iVar3 + 0x9c) = uVar1;
  return 0;
}


//==== pCus_SetAEGain @ 00010988

undefined4 pCus_SetAEGain(int param_1,uint param_2)

{
  undefined2 uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint *puVar5;
  ushort uVar6;
  uint uVar7;
  int iVar8;
  bool bVar9;
  longlong lVar10;
  longlong lVar11;
  
  puVar5 = DAT_00010a20;
  uVar2 = DAT_00010a1c;
  iVar8 = *(int *)(param_1 + 0x28);
  bVar9 = param_2 <= DAT_00010a1c;
  *(uint *)(iVar8 + 0x50) = param_2;
  if (bVar9) {
    uVar2 = param_2;
  }
  iVar4 = 0;
  if (uVar2 < 0x400) {
    uVar2 = 0x400;
  }
  do {
    uVar7 = *puVar5;
    uVar3 = uVar2;
    if ((uVar7 == 0) || ((uVar7 < uVar2 && (uVar3 = uVar7, uVar2 < puVar5[1] + uVar7)))) break;
    iVar4 = iVar4 + 1;
    puVar5 = puVar5 + 2;
    uVar3 = uVar2;
  } while (iVar4 != 0x10);
  if (uVar3 < 0x598d) {
    uVar6 = *(ushort *)(iVar8 + 0xa0) & 0xffef;
  }
  else {
    uVar6 = *(ushort *)(iVar8 + 0xa0) | 0x10;
    uVar3 = uVar3 >> 1;
  }
  *(ushort *)(iVar8 + 0xa0) = uVar6;
  lVar10 = intlog10(uVar3);
  lVar11 = intlog10(0x400);
  lVar10 = (lVar10 - lVar11) * 200;
  uVar2 = (uint)((ulonglong)lVar10 >> 0x20);
  uVar1 = __aeabi_uldivmod((uint)lVar10 >> 0x18 | uVar2 * 0x100,uVar2 >> 0x18,3,0);
  *(undefined1 *)(iVar8 + 0x8c) = 1;
  *(undefined2 *)(iVar8 + 0x9c) = uVar1;
  return 0;
}


//==== pCus_GetSensorID @ 00010a24

undefined4 pCus_GetSensorID(undefined4 param_1,undefined4 *param_2)

{
  *param_2 = 0;
  CamOsMsSleep(1);
  CamOsMsSleep(1);
  CamOsMsSleep(1);
  return 0xffffffff;
}


//==== pCus_init_mipi4lane_linear @ 00010a42

undefined4 pCus_init_mipi4lane_linear(int param_1)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  short *local_34;
  
  iVar4 = *(int *)(param_1 + 0x28);
  iVar1 = FUN_000108f8();
  if (iVar1 == 0) {
    local_34 = DAT_00010ad8;
    iVar1 = 0;
    do {
      if (*local_34 == -1) {
        CamOsMsSleep(local_34[1]);
      }
      else {
        iVar5 = 10;
        while (iVar2 = (**(code **)(*(int *)(param_1 + 0x40) + 0x14))
                                 (*(int *)(param_1 + 0x40),param_1 + 0x30), iVar2 != 0) {
          iVar5 = iVar5 + -1;
          if (iVar5 == 0) goto LAB_00010a54;
        }
      }
      iVar1 = iVar1 + 1;
      local_34 = local_34 + 2;
    } while (iVar1 != 0x47);
    iVar1 = *(int *)(iVar4 + 0x38);
    *(ushort *)(iVar4 + 0x90) = (ushort)((uint)(iVar1 << 0xe) >> 0x1e);
    *(ushort *)(iVar4 + 0x94) = (ushort)(byte)((uint)iVar1 >> 8);
    *(ushort *)(iVar4 + 0x98) = (ushort)iVar1 & 0xff;
    *(undefined1 *)(*(int *)(param_1 + 0x28) + 0xd2) = 1;
    uVar3 = 0;
  }
  else {
LAB_00010a54:
    uVar3 = 0xffffffff;
  }
  return uVar3;
}


//==== pCus_init_mipi2lane_HDR_DOL @ 00010adc

undefined4 pCus_init_mipi2lane_HDR_DOL(int param_1)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  short *psVar5;
  int iVar6;
  
  iVar4 = *(int *)(param_1 + 0x28);
  iVar1 = FUN_000108f8();
  if (iVar1 == 0) {
    iVar1 = 0;
    psVar5 = DAT_00010b64;
    do {
      if (*psVar5 == -1) {
        CamOsMsSleep(psVar5[1]);
      }
      else {
        iVar6 = 10;
        while (iVar2 = (**(code **)(*(int *)(param_1 + 0x40) + 0x14))
                                 (*(int *)(param_1 + 0x40),param_1 + 0x30), iVar2 != 0) {
          iVar6 = iVar6 + -1;
          if (iVar6 == 0) goto LAB_00010aee;
        }
      }
      iVar1 = iVar1 + 1;
      psVar5 = psVar5 + 2;
    } while (iVar1 != 0x48);
    iVar1 = *(int *)(iVar4 + 0x38);
    *(ushort *)(iVar4 + 0x90) = (ushort)((uint)(iVar1 << 0xe) >> 0x1e);
    *(ushort *)(iVar4 + 0x94) = (ushort)(byte)((uint)iVar1 >> 8);
    *(ushort *)(iVar4 + 0x98) = (ushort)iVar1 & 0xff;
    uVar3 = 0;
  }
  else {
LAB_00010aee:
    uVar3 = 0xffffffff;
  }
  return uVar3;
}


//==== pCus_init_mipi4lane_HDR_DOL @ 00010b68

undefined4 pCus_init_mipi4lane_HDR_DOL(int param_1)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  short *psVar5;
  int iVar6;
  
  iVar4 = *(int *)(param_1 + 0x28);
  iVar1 = FUN_000108f8();
  if (iVar1 == 0) {
    iVar1 = 0;
    psVar5 = DAT_00010bf0;
    do {
      if (*psVar5 == -1) {
        CamOsMsSleep(psVar5[1]);
      }
      else {
        iVar6 = 10;
        while (iVar2 = (**(code **)(*(int *)(param_1 + 0x40) + 0x14))
                                 (*(int *)(param_1 + 0x40),param_1 + 0x30), iVar2 != 0) {
          iVar6 = iVar6 + -1;
          if (iVar6 == 0) goto LAB_00010b7a;
        }
      }
      iVar1 = iVar1 + 1;
      psVar5 = psVar5 + 2;
    } while (iVar1 != 0x57);
    iVar1 = *(int *)(iVar4 + 0x38);
    *(ushort *)(iVar4 + 0x90) = (ushort)((uint)(iVar1 << 0xe) >> 0x1e);
    *(ushort *)(iVar4 + 0x94) = (ushort)(byte)((uint)iVar1 >> 8);
    *(ushort *)(iVar4 + 0x98) = (ushort)iVar1 & 0xff;
    uVar3 = 0;
  }
  else {
LAB_00010b7a:
    uVar3 = 0xffffffff;
  }
  return uVar3;
}


//==== pCus_poweron @ 00010bf4

undefined4 pCus_poweron(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 0x44);
  (**(code **)(iVar1 + 8))
            (param_2,*(int *)(param_1 + 0x5c) == 0,param_3,*(code **)(iVar1 + 8),param_4);
  (**(code **)(iVar1 + 0xc))(param_2,*(int *)(param_1 + 0x58) == 0);
  (**(code **)(iVar1 + 0x1c))
            (param_2,*(undefined4 *)(param_1 + 0x78),*(undefined4 *)(param_1 + 0x9cc));
  (**(code **)(iVar1 + 0x20))(param_2,4);
  (**(code **)(iVar1 + 0x24))(param_2,*(undefined2 *)(param_1 + 0x9cc),1);
  (**(code **)(iVar1 + 0x28))(param_2,0,0x1c00);
  if (*(int *)(param_1 + 0x9e0) == 1) {
    (**(code **)(iVar1 + 0x34))(param_2,1);
  }
  (**(code **)(iVar1 + 0xc))(param_2,*(undefined4 *)(param_1 + 0x58));
  (**(code **)(iVar1 + 0x10))(param_2,1,*(undefined4 *)(param_1 + 0x9fc));
  CamOsUsDelay(0x14);
  (**(code **)(iVar1 + 8))(param_2,*(undefined4 *)(param_1 + 0x5c));
  CamOsUsDelay(0x14);
  (**(code **)(iVar1 + 8))(param_2,*(int *)(param_1 + 0x5c) == 0);
  CamOsUsDelay(0x14);
  (**(code **)(iVar1 + 0xc))(param_2,*(int *)(param_1 + 0x58) == 0);
  CamOsUsDelay(0x14);
  return 0;
}


//==== pCus_init_mipi2lane_linear @ 00010caa

undefined4 pCus_init_mipi2lane_linear(int param_1,int param_2,undefined4 param_3)

{
  short sVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  short *psVar6;
  
  iVar4 = param_1;
  iVar3 = FUN_000108f8();
  if (iVar3 == 0) {
    iVar3 = 0;
    psVar6 = DAT_00010d3c;
    do {
      sVar1 = *psVar6;
      sVar2 = psVar6[1];
      if (sVar1 == -1) {
        CamOsMsSleep(sVar2);
      }
      else {
        param_2 = 10;
        while (iVar5 = (**(code **)(*(int *)(param_1 + 0x40) + 0x14))
                                 (*(int *)(param_1 + 0x40),param_1 + 0x30,sVar1,sVar2,iVar4,param_2,
                                  param_3), iVar5 != 0) {
          param_2 = param_2 + -1;
          if (param_2 == 0) {
            return 0xffffffff;
          }
        }
      }
      iVar3 = iVar3 + 1;
      psVar6 = psVar6 + 2;
    } while (iVar3 != 0x2c);
    iVar4 = (**(code **)(*(int *)(param_1 + 0x40) + 0x14))
                      (*(int *)(param_1 + 0x40),param_1 + 0x30,*DAT_00010d40,DAT_00010d40[1],iVar4,
                       param_2);
    if ((iVar4 == 0) &&
       (iVar4 = (**(code **)(*(int *)(param_1 + 0x40) + 0x14))
                          (*(int *)(param_1 + 0x40),param_1 + 0x30,0x3002,0), iVar4 == 0)) {
      *(undefined1 *)(*(int *)(param_1 + 0x28) + 0xd2) = 1;
      return 0;
    }
  }
  return 0xffffffff;
}


//==== FUN_00010d44 @ 00010d44

undefined4 FUN_00010d44(char *param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = strcmp(param_1,DAT_00010d9c);
  uVar2 = 0;
  if (iVar1 != 0) {
    iVar1 = strcmp(param_1,DAT_00010da0);
    if (iVar1 == 0) {
      uVar2 = 2;
    }
    else {
      iVar1 = strcmp(param_1,DAT_00010da4);
      if (iVar1 == 0) {
        uVar2 = 4;
      }
      else {
        iVar1 = strcmp(param_1,DAT_00010da8);
        if (iVar1 == 0) {
          uVar2 = 9;
        }
        else {
          iVar1 = strcmp(param_1,DAT_00010dac);
          if (iVar1 == 0) {
            uVar2 = 5;
          }
          else {
            iVar1 = strcmp(param_1,DAT_00010db0);
            if (iVar1 == 0) {
              uVar2 = 10;
            }
            else {
              uVar2 = 0xb;
            }
          }
        }
      }
    }
  }
  return uVar2;
}


//==== cus_camsensor_init_handle @ 00010db4

undefined4 cus_camsensor_init_handle(int param_1)

{
  int iVar1;
  char *pcVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  
  pcVar2 = DAT_00010ff0;
  iVar1 = DAT_00010fec;
  iVar6 = DAT_00010fe8;
  if (param_1 != 0) {
    iVar5 = *(int *)(param_1 + 0x28);
    if (iVar5 != 0) {
      *(undefined4 *)(iVar5 + 0x9a) = *(undefined4 *)(DAT_00010fe8 + 0x462);
      *(undefined4 *)(iVar5 + 0x9e) = *(undefined4 *)(iVar6 + 0x466);
      *(undefined4 *)(iVar5 + 0xa2) = *(undefined4 *)(iVar6 + 0x46a);
      *(undefined4 *)(iVar5 + 0xa6) = *(undefined4 *)(iVar6 + 0x46e);
      *(undefined4 *)(iVar5 + 0xaa) = *(undefined4 *)(iVar6 + 0x472);
      *(undefined4 *)(iVar5 + 0xae) = *(undefined4 *)(iVar6 + 0x476);
      *(undefined4 *)(iVar5 + 0x8e) = *(undefined4 *)(iVar6 + 0x47a);
      *(undefined4 *)(iVar5 + 0x92) = *(undefined4 *)(iVar6 + 0x47e);
      *(undefined4 *)(iVar5 + 0x96) = *(undefined4 *)(iVar6 + 0x482);
      *(undefined4 *)(iVar5 + 0xbe) = *(undefined4 *)(iVar6 + 0x486);
      *(undefined4 *)(iVar5 + 0xc2) = *(undefined4 *)(iVar6 + 0x48a);
      *(undefined4 *)(iVar5 + 0xc6) = *(undefined4 *)(iVar6 + 0x48e);
      *(undefined4 *)(iVar5 + 0xb2) = *(undefined4 *)(iVar6 + 0x492);
      *(undefined4 *)(iVar5 + 0xb6) = *(undefined4 *)(iVar6 + 0x496);
      *(undefined4 *)(iVar5 + 0xba) = *(undefined4 *)(iVar6 + 0x49a);
      *(undefined4 *)(iVar5 + 0xca) = *(undefined4 *)(iVar6 + 0x49e);
      *(undefined4 *)(iVar5 + 0xce) = *(undefined4 *)(iVar6 + 0x4a2);
      strcpy((char *)(param_1 + 4),pcVar2);
      pcVar2 = DAT_00010ff4;
      iVar6 = *(int *)(iVar1 + 4);
      *(undefined4 *)(param_1 + 0x6c) = 1;
      *(undefined4 *)(param_1 + 0x78) = 1;
      *(undefined4 *)(param_1 + 0x7c) = 1;
      *(undefined4 *)(param_1 + 0x80) = 4;
      *(undefined4 *)(param_1 + 0x9d0) = 1;
      *(undefined4 *)(param_1 + 0x90) = 1;
      *(undefined4 *)(param_1 + 0x94) = 0;
      *(undefined4 *)(param_1 + 0x70) = 0;
      *(undefined4 *)(param_1 + 0x74) = 0;
      *(undefined4 *)(param_1 + 0x98) = 0x780;
      *(undefined4 *)(param_1 + 0x9c) = 0x438;
      *(undefined4 *)(param_1 + 0x84) = 0;
      *(undefined4 *)(param_1 + 0xa0) = 0x1e;
      *(undefined4 *)(param_1 + 0xa4) = 3;
      *(undefined4 *)(param_1 + 0x9d4) = 0;
      *(undefined4 *)(param_1 + 0x9d8) = 0;
      *(undefined4 *)(param_1 + 0x9e0) = 0;
      *(undefined4 *)(param_1 + 0x9e4) = 0;
      *(undefined4 *)(param_1 + 0xa8) = 0;
      *(undefined4 *)(param_1 + 0xac) = 0;
      *(int *)(param_1 + 0x9cc) = iVar6;
      *(undefined4 *)(param_1 + 0xb0) = 0x79c;
      *(undefined4 *)(param_1 + 0xb4) = 0x449;
      *(undefined4 *)(param_1 + 0xd8) = 0x465;
      *(undefined4 *)(param_1 + 0xdc) = 0xe77a;
      if (iVar6 == 4) {
        *(undefined4 *)(param_1 + 0xa0) = 0x3c;
      }
      strcpy((char *)(param_1 + 0xb8),pcVar2);
      uVar3 = *(undefined4 *)(iVar1 + 0xc);
      *(undefined4 *)(param_1 + 0x30) = 1;
      *(undefined4 *)(param_1 + 0x34) = 1;
      *(undefined2 *)(param_1 + 0x3c) = 0x34;
      *(undefined4 *)(param_1 + 0x38) = 20000;
      uVar4 = FUN_00010d44(uVar3);
      *(undefined4 *)(param_1 + 0x4c) = 2;
      *(undefined4 *)(param_1 + 0x50) = 1;
      *(undefined4 *)(param_1 + 0x54) = 2;
      *(undefined4 *)(param_1 + 0xa00) = 0x400;
      *(undefined4 *)(param_1 + 0xa08) = 0xb54;
      uVar3 = DAT_00010ff8;
      *(undefined4 *)(param_1 + 0x58) = 1;
      *(undefined4 *)(param_1 + 0x5c) = 1;
      *(undefined4 *)(param_1 + 0x68) = 1;
      *(undefined4 *)(param_1 + 0xa1c) = uVar3;
      uVar3 = DAT_00010ffc;
      if (iVar6 == 4) {
        uVar3 = DAT_00011000;
      }
      *(undefined4 *)(param_1 + 0x48) = 1;
      *(undefined4 *)(param_1 + 0xa14) = uVar3;
      *(undefined4 *)(param_1 + 0xa0c) = DAT_00011004;
      *(undefined4 *)(param_1 + 0xa10) = DAT_00011008;
      *(undefined4 *)(param_1 + 0xa2c) = DAT_0001100c;
      *(undefined4 *)(param_1 + 0xa78) = DAT_00011010;
      uVar3 = DAT_00011014;
      *(undefined4 *)(param_1 + 0x9fc) = uVar4;
      *(undefined4 *)(param_1 + 0xa30) = uVar3;
      *(undefined4 *)(param_1 + 0xa34) = DAT_00011018;
      *(undefined4 *)(param_1 + 0xa38) = DAT_0001101c;
      *(undefined4 *)(param_1 + 0xa3c) = DAT_00011020;
      *(undefined4 *)(param_1 + 0xa40) = DAT_00011024;
      *(undefined4 *)(param_1 + 0xa64) = DAT_00011028;
      *(undefined4 *)(param_1 + 0xa68) = DAT_0001102c;
      *(undefined4 *)(param_1 + 0xa28) = DAT_00011030;
      *(undefined4 *)(param_1 + 0xa48) = DAT_00011034;
      *(undefined4 *)(param_1 + 0xa4c) = DAT_00011038;
      *(undefined4 *)(param_1 + 0xa50) = DAT_0001103c;
      *(undefined4 *)(param_1 + 0xa54) = DAT_00011040;
      *(undefined4 *)(param_1 + 0xa58) = DAT_00011044;
      *(undefined4 *)(param_1 + 0xa60) = DAT_00011048;
      *(undefined4 *)(param_1 + 0xa5c) = DAT_0001104c;
      *(undefined4 *)(param_1 + 0xa6c) = DAT_00011050;
      *(undefined4 *)(param_1 + 0xa70) = DAT_00011054;
      *(undefined4 *)(param_1 + 0xa74) = DAT_00011058;
      *(undefined4 *)(iVar5 + 0x38) = 0x465;
      *(undefined4 *)(iVar5 + 0x60) = 0x2a1;
      iVar6 = *(int *)(param_1 + 0x9fc);
      *(undefined1 *)(iVar5 + 0x8c) = 0;
      if (iVar6 == 4) {
        uVar3 = 0x1d;
      }
      else {
        uVar3 = 0x1e;
      }
      *(undefined4 *)(iVar5 + 0x58) = uVar3;
      return 0;
    }
    CamOsPrintf(DAT_00010fe4,DAT_00010fe0);
  }
  return 0xffffffff;
}


//==== cus_camsensor_init_handle_hdr_dol_sef1 @ 0001105c

undefined4 cus_camsensor_init_handle_hdr_dol_sef1(int param_1)

{
  char *__src;
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  
  cus_camsensor_init_handle();
  iVar3 = *(int *)(param_1 + 0x28);
  strcpy((char *)(param_1 + 4),DAT_0001112c);
  iVar2 = DAT_00011130;
  *(undefined4 *)(param_1 + 0x70) = 0;
  *(undefined4 *)(param_1 + 0x74) = 0;
  iVar2 = *(int *)(iVar2 + 8);
  *(undefined4 *)(param_1 + 0xa8) = 0;
  *(undefined4 *)(param_1 + 0xac) = 0;
  *(undefined4 *)(param_1 + 0x90) = 1;
  *(undefined4 *)(param_1 + 0x94) = 0;
  *(undefined4 *)(param_1 + 0x9e0) = 1;
  *(undefined4 *)(param_1 + 0xb0) = 0x79c;
  *(undefined4 *)(param_1 + 0xb4) = 0x449;
  *(undefined4 *)(param_1 + 0x98) = 0x780;
  *(undefined4 *)(param_1 + 0x9c) = 0x438;
  *(undefined4 *)(param_1 + 0x7c) = 3;
  *(undefined4 *)(param_1 + 0xa0) = 0x1e;
  *(undefined4 *)(param_1 + 0xa4) = 3;
  *(int *)(param_1 + 0x9cc) = iVar2;
  __src = DAT_00011134;
  *(undefined4 *)(param_1 + 0x9d8) = 3;
  strcpy((char *)(param_1 + 0xb8),__src);
  *(undefined4 *)(param_1 + 0xa38) = DAT_00011138;
  uVar1 = DAT_0001113c;
  if (iVar2 == 2) {
    uVar1 = DAT_00011140;
  }
  *(undefined4 *)(param_1 + 0xa14) = uVar1;
  *(undefined4 *)(param_1 + 0xa64) = DAT_00011144;
  *(undefined4 *)(param_1 + 0xa68) = DAT_00011148;
  *(undefined4 *)(param_1 + 0xa3c) = DAT_0001114c;
  *(undefined4 *)(param_1 + 0xa40) = DAT_00011150;
  *(undefined4 *)(param_1 + 0xa48) = DAT_00011154;
  *(undefined4 *)(param_1 + 0xa4c) = DAT_00011158;
  *(undefined4 *)(param_1 + 0xa50) = DAT_0001115c;
  *(undefined4 *)(param_1 + 0xa54) = DAT_00011160;
  *(undefined4 *)(param_1 + 0xa58) = DAT_00011164;
  *(undefined4 *)(param_1 + 0xa74) = DAT_00011168;
  *(undefined4 *)(iVar3 + 0x38) = 0x486;
  *(undefined4 *)(iVar3 + 0x60) = 0x2a1;
  *(undefined4 *)(param_1 + 0x9e4) = 1;
  *(undefined4 *)(param_1 + 0x48) = 2;
  *(undefined4 *)(param_1 + 0x4c) = 2;
  *(undefined4 *)(param_1 + 0x50) = 2;
  *(undefined4 *)(param_1 + 0x54) = 2;
  return 0;
}


//==== cus_camsensor_init_handle_hdr_dol_lef @ 0001116c

undefined4 cus_camsensor_init_handle_hdr_dol_lef(int param_1)

{
  char *pcVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  
  iVar2 = DAT_000113c0;
  pcVar1 = DAT_00011368;
  iVar5 = DAT_00011364;
  if (param_1 != 0) {
    iVar6 = *(int *)(param_1 + 0x28);
    if (iVar6 != 0) {
      *(undefined4 *)(iVar6 + 0x9a) = *(undefined4 *)(DAT_00011364 + 0x462);
      *(undefined4 *)(iVar6 + 0x9e) = *(undefined4 *)(iVar5 + 0x466);
      *(undefined4 *)(iVar6 + 0xa2) = *(undefined4 *)(iVar5 + 0x46a);
      *(undefined4 *)(iVar6 + 0xa6) = *(undefined4 *)(iVar5 + 0x46e);
      *(undefined4 *)(iVar6 + 0xaa) = *(undefined4 *)(iVar5 + 0x472);
      *(undefined4 *)(iVar6 + 0xae) = *(undefined4 *)(iVar5 + 0x476);
      *(undefined4 *)(iVar6 + 0x8e) = *(undefined4 *)(iVar5 + 0x47a);
      *(undefined4 *)(iVar6 + 0x92) = *(undefined4 *)(iVar5 + 0x47e);
      *(undefined4 *)(iVar6 + 0x96) = *(undefined4 *)(iVar5 + 0x482);
      *(undefined4 *)(iVar6 + 0xbe) = *(undefined4 *)(iVar5 + 0x486);
      *(undefined4 *)(iVar6 + 0xc2) = *(undefined4 *)(iVar5 + 0x48a);
      *(undefined4 *)(iVar6 + 0xc6) = *(undefined4 *)(iVar5 + 0x48e);
      *(undefined4 *)(iVar6 + 0xb2) = *(undefined4 *)(iVar5 + 0x492);
      *(undefined4 *)(iVar6 + 0xb6) = *(undefined4 *)(iVar5 + 0x496);
      *(undefined4 *)(iVar6 + 0xba) = *(undefined4 *)(iVar5 + 0x49a);
      *(undefined4 *)(iVar6 + 0xca) = *(undefined4 *)(iVar5 + 0x49e);
      *(undefined4 *)(iVar6 + 0xce) = *(undefined4 *)(iVar5 + 0x4a2);
      strcpy((char *)(param_1 + 4),pcVar1);
      *(undefined4 *)(param_1 + 0x7c) = 3;
      *(undefined4 *)(param_1 + 0x80) = 4;
      uVar4 = *(undefined4 *)(iVar2 + 8);
      *(undefined4 *)(param_1 + 0x9d8) = 3;
      *(undefined4 *)(param_1 + 0x6c) = 1;
      *(undefined4 *)(param_1 + 0x9cc) = uVar4;
      *(undefined4 *)(param_1 + 0x78) = 1;
      *(undefined4 *)(param_1 + 0x98) = 0x780;
      *(undefined4 *)(param_1 + 0x9c) = 0x438;
      pcVar1 = DAT_0001136c;
      *(undefined4 *)(param_1 + 0xa0) = 0x1e;
      *(undefined4 *)(param_1 + 0xa4) = 3;
      *(undefined4 *)(param_1 + 0x70) = 0;
      *(undefined4 *)(param_1 + 0x74) = 0;
      *(undefined4 *)(param_1 + 0xb0) = 0x79c;
      *(undefined4 *)(param_1 + 0xb4) = 0x449;
      *(undefined4 *)(param_1 + 0x84) = 0;
      *(undefined4 *)(param_1 + 0x9d0) = 1;
      *(undefined4 *)(param_1 + 0x9d4) = 0;
      *(undefined4 *)(param_1 + 0x9e0) = 1;
      *(undefined4 *)(param_1 + 0x9e4) = 0;
      *(undefined4 *)(param_1 + 0x90) = 1;
      *(undefined4 *)(param_1 + 0x94) = 0;
      *(undefined4 *)(param_1 + 0xa8) = 0;
      *(undefined4 *)(param_1 + 0xac) = 0;
      strcpy((char *)(param_1 + 0xb8),pcVar1);
      uVar4 = *(undefined4 *)(iVar2 + 0xc);
      *(undefined4 *)(param_1 + 0x30) = 1;
      *(undefined4 *)(param_1 + 0x34) = 1;
      *(undefined2 *)(param_1 + 0x3c) = 0x34;
      *(undefined4 *)(param_1 + 0x38) = 20000;
      uVar3 = FUN_00010d44(uVar4);
      *(undefined4 *)(param_1 + 0x48) = 2;
      *(undefined4 *)(param_1 + 0x4c) = 2;
      *(undefined4 *)(param_1 + 0x50) = 2;
      *(undefined4 *)(param_1 + 0x54) = 2;
      *(undefined4 *)(param_1 + 0xa00) = 0x400;
      uVar4 = DAT_00011370;
      *(undefined4 *)(param_1 + 0x58) = 1;
      *(undefined4 *)(param_1 + 0x5c) = 1;
      *(undefined4 *)(param_1 + 0xa78) = 0;
      *(undefined4 *)(param_1 + 0xa1c) = uVar4;
      uVar4 = DAT_00011374;
      *(undefined4 *)(param_1 + 0xa30) = 0;
      *(undefined4 *)(param_1 + 0xa34) = 0;
      *(undefined4 *)(param_1 + 0xa14) = uVar4;
      uVar4 = DAT_00011378;
      *(undefined4 *)(param_1 + 0xa38) = 0;
      *(undefined4 *)(param_1 + 0xa0c) = uVar4;
      *(undefined4 *)(param_1 + 0xa10) = DAT_0001137c;
      *(undefined4 *)(param_1 + 0xa2c) = DAT_00011380;
      *(undefined4 *)(param_1 + 0xa3c) = DAT_00011384;
      *(undefined4 *)(param_1 + 0xa40) = DAT_00011388;
      uVar4 = DAT_0001138c;
      *(undefined4 *)(param_1 + 0x9fc) = uVar3;
      *(undefined4 *)(param_1 + 0xa64) = uVar4;
      *(undefined4 *)(param_1 + 0xa68) = DAT_00011390;
      *(undefined4 *)(param_1 + 0xa28) = DAT_00011394;
      *(undefined4 *)(param_1 + 0xa48) = DAT_00011398;
      *(undefined4 *)(param_1 + 0xa4c) = DAT_0001139c;
      *(undefined4 *)(param_1 + 0xa50) = DAT_000113a0;
      *(undefined4 *)(param_1 + 0xa54) = DAT_000113a4;
      *(undefined4 *)(param_1 + 0xa58) = DAT_000113a8;
      *(undefined4 *)(param_1 + 0xa60) = DAT_000113ac;
      *(undefined4 *)(param_1 + 0xa5c) = DAT_000113b0;
      *(undefined4 *)(param_1 + 0xa6c) = DAT_000113b4;
      *(undefined4 *)(param_1 + 0xa70) = DAT_000113b8;
      *(undefined4 *)(param_1 + 0xa74) = DAT_000113bc;
      *(undefined4 *)(iVar6 + 0x38) = 0x486;
      *(undefined4 *)(iVar6 + 0x60) = 0x2a1;
      iVar5 = *(int *)(param_1 + 0x9fc);
      *(undefined1 *)(iVar6 + 0x8c) = 0;
      if (iVar5 == 4) {
        uVar4 = 0x1d;
      }
      else {
        uVar4 = 0x1e;
      }
      *(undefined4 *)(iVar6 + 0x58) = uVar4;
      return 0;
    }
    CamOsPrintf(DAT_00011360,DAT_0001135c);
  }
  return 0xffffffff;
}


//==== pCus_setCaliData_gain_linearity_hdr_dol_lef @ 000113c4

undefined4 pCus_setCaliData_gain_linearity_hdr_dol_lef(void)

{
  return 0;
}


//==== pCus_GetAEGain_isp_calibration @ 000113c8

void pCus_GetAEGain_isp_calibration(undefined4 param_1,undefined4 *param_2)

{
  *param_2 = 0;
  return;
}


//==== pCus_init_hdr_dol_lef @ 000113ce

undefined4 pCus_init_hdr_dol_lef(void)

{
  return 0;
}


//==== FUN_000113d2 @ 000113d2

void FUN_000113d2(uint param_1,undefined2 *param_2)

{
  undefined2 uVar1;
  uint uVar2;
  longlong lVar3;
  longlong lVar4;
  
  if (DAT_0001141c <= param_1) {
    param_1 = DAT_0001141c;
  }
  if (param_1 < 0x400) {
    param_1 = 0x400;
  }
  lVar3 = intlog10(param_1);
  lVar4 = intlog10(0x400);
  lVar3 = (lVar3 - lVar4) * 200;
  uVar2 = (uint)((ulonglong)lVar3 >> 0x20);
  uVar1 = __aeabi_uldivmod((uint)lVar3 >> 0x18 | uVar2 * 0x100,uVar2 >> 0x18,3,0);
  *param_2 = uVar1;
  return;
}


//==== pCus_SetAEGainHDR_DOL_LEF @ 00011420

undefined4 pCus_SetAEGainHDR_DOL_LEF(int param_1,uint param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 uStack_14;
  undefined4 uStack_10;
  
  iVar1 = *(int *)(param_1 + 0x28);
  uStack_14 = param_2 & 0xffff;
  *(uint *)(iVar1 + 0x6c) = param_2;
  uStack_10 = param_3;
  FUN_000113d2(param_2,(int)&uStack_14 + 2,param_3,param_1,param_1);
  *(undefined2 *)(iVar1 + 0xcc) = uStack_14._2_2_;
  *(undefined1 *)(iVar1 + 0x8c) = 1;
  return 0;
}


//==== pCus_SetAEGainHDR_DOL_SEF1 @ 0001144c

undefined4 pCus_SetAEGainHDR_DOL_SEF1(int param_1,uint param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 uStack_14;
  undefined4 uStack_10;
  
  iVar1 = *(int *)(param_1 + 0x28);
  uStack_14 = param_2 & 0xffff;
  *(uint *)(iVar1 + 0x70) = param_2;
  uStack_10 = param_3;
  FUN_000113d2(param_2,(int)&uStack_14 + 2,param_3,param_1,param_1);
  *(undefined2 *)(iVar1 + 0xd0) = uStack_14._2_2_;
  *(undefined1 *)(iVar1 + 0x8c) = 1;
  return 0;
}


//==== pCus_SetAEGain_cal_hdr_dol_lef @ 00011478

undefined4 pCus_SetAEGain_cal_hdr_dol_lef(void)

{
  return 0;
}


//==== pCus_poweron_isp_calibration @ 0001147c

undefined4 pCus_poweron_isp_calibration(void)

{
  return 0;
}


//==== pCus_poweroff_isp_calibration @ 00011480

undefined4 pCus_poweroff_isp_calibration(void)

{
  return 0;
}


//==== imx307_SetPatternMode_isp_calibration @ 00011484

undefined4 imx307_SetPatternMode_isp_calibration(void)

{
  return 0;
}


//==== pCus_SetOrien_isp_calibration @ 00011488

undefined4 pCus_SetOrien_isp_calibration(void)

{
  return 0;
}


//==== pCus_SetFPS_isp_calibration @ 0001148c

undefined4 pCus_SetFPS_isp_calibration(void)

{
  return 0;
}


//==== pCus_AEStatusNotify_isp_calibration @ 00011490

undefined4 pCus_AEStatusNotify_isp_calibration(void)

{
  return 0;
}


//==== pCus_SetAEUSecs_isp_calibration @ 00011494

undefined4 pCus_SetAEUSecs_isp_calibration(void)

{
  return 0;
}


//==== pCus_SetAEGain_cal_isp_calibration @ 00011498

undefined4 pCus_SetAEGain_cal_isp_calibration(void)

{
  return 0;
}


//==== pCus_SetAEGain_isp_calibration @ 0001149c

undefined4 pCus_SetAEGain_isp_calibration(void)

{
  return 0;
}


//==== pCus_poweron_hdr_dol_lef @ 000114a0

undefined4 pCus_poweron_hdr_dol_lef(void)

{
  return 0;
}


//==== pCus_poweroff_hdr_dol_lef @ 000114a4

undefined4 pCus_poweroff_hdr_dol_lef(void)

{
  return 0;
}


//==== imx307_SetPatternMode_hdr_dol_lef @ 000114a8

undefined4 imx307_SetPatternMode_hdr_dol_lef(void)

{
  return 0;
}


//==== pCus_GetAEMinMaxUSecs_hdr_dol_lef @ 000114ac

undefined4
pCus_GetAEMinMaxUSecs_hdr_dol_lef(undefined4 param_1,undefined4 *param_2,undefined4 *param_3)

{
  *param_2 = 1;
  *param_3 = DAT_000114b8;
  return 0;
}


//==== pCus_GetSensorID_hdr_dol_lef @ 000114bc

void pCus_GetSensorID_hdr_dol_lef(undefined4 param_1,undefined4 *param_2)

{
  *param_2 = 0;
  return;
}


//==== pCus_GetAEUSecs_hdr_dol_lef @ 000114c2

void pCus_GetAEUSecs_hdr_dol_lef(undefined4 param_1,undefined4 *param_2)

{
  *param_2 = 0;
  return;
}


//==== pCus_init_mipi4lane_linear_isp_calibration @ 000114c8

undefined4 pCus_init_mipi4lane_linear_isp_calibration(void)

{
  return 0;
}


//==== pCus_init_mipi2lane_linear_isp_calibration @ 000114cc

undefined4 pCus_init_mipi2lane_linear_isp_calibration(void)

{
  return 0;
}


//==== pCus_GetFPS_isp_calibration @ 000114d0

undefined4 pCus_GetFPS_isp_calibration(void)

{
  return 0;
}


//==== pCus_GetAEUSecs_isp_calibration @ 000114d4

void pCus_GetAEUSecs_isp_calibration(undefined4 param_1,undefined4 *param_2)

{
  *param_2 = 0;
  return;
}


//==== cus_camsensor_init_handle_linear_isp_calibration @ 000114da

undefined4
cus_camsensor_init_handle_linear_isp_calibration
          (int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  char *pcVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  
  iVar2 = DAT_0001174c;
  pcVar1 = DAT_000116e4;
  iVar6 = DAT_000116e0;
  if (param_1 != 0) {
    iVar5 = *(int *)(param_1 + 0x28);
    if (iVar5 != 0) {
      *(undefined4 *)(iVar5 + 0x9a) = *(undefined4 *)(DAT_000116e0 + 0x462);
      *(undefined4 *)(iVar5 + 0x9e) = *(undefined4 *)(iVar6 + 0x466);
      *(undefined4 *)(iVar5 + 0xa2) = *(undefined4 *)(iVar6 + 0x46a);
      *(undefined4 *)(iVar5 + 0xa6) = *(undefined4 *)(iVar6 + 0x46e);
      *(undefined4 *)(iVar5 + 0xaa) = *(undefined4 *)(iVar6 + 0x472);
      *(undefined4 *)(iVar5 + 0xae) = *(undefined4 *)(iVar6 + 0x476);
      *(undefined4 *)(iVar5 + 0x8e) = *(undefined4 *)(iVar6 + 0x47a);
      *(undefined4 *)(iVar5 + 0x92) = *(undefined4 *)(iVar6 + 0x47e);
      *(undefined4 *)(iVar5 + 0x96) = *(undefined4 *)(iVar6 + 0x482);
      *(undefined4 *)(iVar5 + 0xbe) = *(undefined4 *)(iVar6 + 0x486);
      *(undefined4 *)(iVar5 + 0xc2) = *(undefined4 *)(iVar6 + 0x48a);
      *(undefined4 *)(iVar5 + 0xc6) = *(undefined4 *)(iVar6 + 0x48e);
      *(undefined4 *)(iVar5 + 0xb2) = *(undefined4 *)(iVar6 + 0x492);
      *(undefined4 *)(iVar5 + 0xb6) = *(undefined4 *)(iVar6 + 0x496);
      *(undefined4 *)(iVar5 + 0xba) = *(undefined4 *)(iVar6 + 0x49a);
      *(undefined4 *)(iVar5 + 0xca) = *(undefined4 *)(iVar6 + 0x49e);
      *(undefined4 *)(iVar5 + 0xce) = *(undefined4 *)(iVar6 + 0x4a2);
      strcpy((char *)(param_1 + 4),pcVar1);
      pcVar1 = DAT_000116e8;
      iVar6 = *(int *)(iVar2 + 4);
      *(undefined4 *)(param_1 + 0x80) = 4;
      *(undefined4 *)(param_1 + 0x70) = 0;
      *(undefined4 *)(param_1 + 0x74) = 0;
      *(undefined4 *)(param_1 + 0x84) = 0;
      *(undefined4 *)(param_1 + 0x9d4) = 0;
      *(undefined4 *)(param_1 + 0x9d8) = 0;
      *(undefined4 *)(param_1 + 0x9e0) = 0;
      *(undefined4 *)(param_1 + 0x6c) = 1;
      *(undefined4 *)(param_1 + 0x78) = 1;
      *(undefined4 *)(param_1 + 0x7c) = 1;
      *(int *)(param_1 + 0x9cc) = iVar6;
      *(undefined4 *)(param_1 + 0x9d0) = 1;
      *(undefined4 *)(param_1 + 0x9e4) = 1;
      strcpy((char *)(param_1 + 0xb8),pcVar1);
      *(undefined4 *)(param_1 + 0x90) = 1;
      *(undefined4 *)(param_1 + 0x94) = 0;
      *(undefined4 *)(param_1 + 0x98) = 0x780;
      *(undefined4 *)(param_1 + 0x9c) = 0x438;
      *(undefined4 *)(param_1 + 0xa8) = 0;
      *(undefined4 *)(param_1 + 0xac) = 0;
      *(undefined4 *)(param_1 + 0xa0) = 0x1e;
      *(undefined4 *)(param_1 + 0xa4) = 3;
      uVar3 = *(undefined4 *)(iVar2 + 0xc);
      *(undefined4 *)(param_1 + 0xb0) = 0x79c;
      *(undefined4 *)(param_1 + 0xb4) = 0x449;
      *(undefined4 *)(param_1 + 0x30) = 1;
      *(undefined4 *)(param_1 + 0x34) = 1;
      *(undefined2 *)(param_1 + 0x3c) = 0x34;
      *(undefined4 *)(param_1 + 0x38) = 20000;
      uVar4 = FUN_00010d44(uVar3);
      *(undefined4 *)(param_1 + 0x4c) = 2;
      *(undefined4 *)(param_1 + 0x50) = 1;
      *(undefined4 *)(param_1 + 0x54) = 2;
      *(undefined4 *)(param_1 + 0xa00) = 0x400;
      uVar3 = DAT_000116ec;
      *(undefined4 *)(param_1 + 0x58) = 1;
      *(undefined4 *)(param_1 + 0x5c) = 1;
      *(undefined4 *)(param_1 + 0x68) = 1;
      *(undefined4 *)(param_1 + 0xa1c) = uVar3;
      uVar3 = DAT_000116f0;
      if (iVar6 == 4) {
        uVar3 = DAT_000116f4;
      }
      *(undefined4 *)(param_1 + 0x48) = 1;
      *(undefined4 *)(param_1 + 0xa14) = uVar3;
      *(undefined4 *)(param_1 + 0xa0c) = DAT_000116f8;
      *(undefined4 *)(param_1 + 0xa10) = DAT_000116fc;
      *(undefined4 *)(param_1 + 0xa2c) = DAT_00011700;
      *(undefined4 *)(param_1 + 0xa78) = DAT_00011704;
      *(undefined4 *)(param_1 + 0xa30) = DAT_00011708;
      uVar3 = DAT_0001170c;
      *(undefined4 *)(param_1 + 0x9fc) = uVar4;
      *(undefined4 *)(param_1 + 0xa38) = uVar3;
      *(undefined4 *)(param_1 + 0xa3c) = DAT_00011710;
      *(undefined4 *)(param_1 + 0xa40) = DAT_00011714;
      *(undefined4 *)(param_1 + 0xa64) = DAT_00011718;
      *(undefined4 *)(param_1 + 0xa68) = DAT_0001171c;
      *(undefined4 *)(param_1 + 0xa28) = DAT_00011720;
      *(undefined4 *)(param_1 + 0xa48) = DAT_00011724;
      *(undefined4 *)(param_1 + 0xa4c) = DAT_00011728;
      *(undefined4 *)(param_1 + 0xa50) = DAT_0001172c;
      *(undefined4 *)(param_1 + 0xa54) = DAT_00011730;
      *(undefined4 *)(param_1 + 0xa58) = DAT_00011734;
      *(undefined4 *)(param_1 + 0xa60) = DAT_00011738;
      *(undefined4 *)(param_1 + 0xa5c) = DAT_0001173c;
      *(undefined4 *)(param_1 + 0xa6c) = DAT_00011740;
      *(undefined4 *)(param_1 + 0xa70) = DAT_00011744;
      *(undefined4 *)(param_1 + 0xa74) = DAT_00011748;
      *(undefined4 *)(iVar5 + 0x38) = 0x465;
      *(undefined4 *)(iVar5 + 0x60) = 0x2a1;
      iVar6 = *(int *)(param_1 + 0x9fc);
      *(undefined1 *)(iVar5 + 0x8c) = 0;
      if (iVar6 == 4) {
        uVar3 = 0x1d;
      }
      else {
        uVar3 = 0x1e;
      }
      *(undefined4 *)(iVar5 + 0x58) = uVar3;
      return 0;
    }
    CamOsPrintf(DAT_000116dc,DAT_000116d8,param_3,param_4,param_4);
  }
  return 0xffffffff;
}


//==== init_module @ 00011750

undefined4 init_module(void)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  int iVar3;
  void *pvVar4;
  undefined4 uVar5;
  uint uVar6;
  undefined4 *puVar7;
  uint uVar8;
  
  iVar3 = DrvSensorHandleVer(2);
  if (((iVar3 == -1) || (iVar3 = DrvSensorIFVer(2,1), iVar3 == -1)) ||
     (iVar3 = DrvSensorI2CVer(1), uVar2 = DAT_00011858, uVar5 = DAT_00011854, puVar1 = DAT_0001183c,
     iVar3 == -1)) {
    uVar5 = 0xffffffff;
  }
  else {
    uVar6 = 0;
    puVar7 = DAT_0001183c;
    do {
      if (((int)puVar1[8] >> (uVar6 & 0xff)) << 0x1f < 0) {
        uVar8 = uVar6 << 3;
        pvVar4 = (void *)CamOsMemAlloc(0xd4);
        memset(pvVar4,0,0xd4);
        DrvRegisterSensorDriverEx(uVar6,uVar5,pvVar4);
        DrvRegisterSensorI2CSlaveID(uVar6,(int)puVar1[9] >> (uVar8 & 0xff) & 0xff);
        printk(DAT_00011840,uVar2,uVar6);
        *puVar7 = pvVar4;
        pvVar4 = (void *)CamOsMemAlloc(0xd4);
        memset(pvVar4,0,0xd4);
        DrvRegisterPlaneDriverEx(uVar6,1,DAT_00011844,pvVar4);
        DrvRegisterPlaneI2CSlaveID(uVar6,1,(int)puVar1[9] >> (uVar8 & 0xff) & 0xff);
        printk(DAT_00011848,uVar2,uVar6);
        DrvRegisterPlaneDriverEx(uVar6,0,DAT_0001184c,pvVar4);
        DrvRegisterPlaneI2CSlaveID(uVar6,0,(int)puVar1[9] >> (uVar8 & 0xff) & 0xff);
        printk(DAT_00011850,uVar2,uVar6);
        puVar7[4] = pvVar4;
      }
      uVar6 = uVar6 + 1;
      puVar7 = puVar7 + 1;
    } while (uVar6 != 4);
    uVar5 = 0;
  }
  return uVar5;
}


//==== cleanup_module @ 0001185c

void cleanup_module(void)

{
  int *piVar1;
  int *piVar2;
  uint uVar3;
  
  piVar1 = DAT_000118a0;
  uVar3 = 0;
  piVar2 = DAT_000118a0;
  do {
    if (((piVar1[8] >> (uVar3 & 0xff)) << 0x1f < 0) && ((*piVar2 != 0 || (piVar2[4] != 0)))) {
      DrvSensorRelease(uVar3);
      if (*piVar2 != 0) {
        CamOsMemRelease();
        *piVar2 = 0;
      }
      if (piVar2[4] != 0) {
        CamOsMemRelease();
        piVar2[4] = 0;
      }
    }
    uVar3 = uVar3 + 1;
    piVar2 = piVar2 + 1;
  } while (uVar3 != 4);
  return;
}


//==== DrvSensorRelease @ 00013000

/* WARNING: Control flow encountered bad instruction data */

void DrvSensorRelease(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== strcpy @ 00013004

/* WARNING: Control flow encountered bad instruction data */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */

char * strcpy(char *__dest,char *__src)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== DrvRegisterSensorDriverEx @ 00013008

/* WARNING: Control flow encountered bad instruction data */

void DrvRegisterSensorDriverEx(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== DrvSensorI2CVer @ 0001300c

/* WARNING: Control flow encountered bad instruction data */

void DrvSensorI2CVer(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== intlog10 @ 00013014

/* WARNING: Control flow encountered bad instruction data */

void intlog10(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== CamOsPrintf @ 00013018

/* WARNING: Control flow encountered bad instruction data */

void CamOsPrintf(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== __aeabi_uldivmod @ 0001301c

/* WARNING: Control flow encountered bad instruction data */

void __aeabi_uldivmod(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== CamOsMsSleep @ 00013020

/* WARNING: Control flow encountered bad instruction data */

void CamOsMsSleep(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== DrvSensorIFVer @ 00013024

/* WARNING: Control flow encountered bad instruction data */

void DrvSensorIFVer(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== printk @ 00013028

/* WARNING: Control flow encountered bad instruction data */

void printk(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== memset @ 0001302c

/* WARNING: Control flow encountered bad instruction data */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void * memset(void *__s,int __c,size_t __n)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== __aeabi_uidiv @ 00013034

/* WARNING: Control flow encountered bad instruction data */

void __aeabi_uidiv(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== DrvRegisterPlaneDriverEx @ 00013038

/* WARNING: Control flow encountered bad instruction data */

void DrvRegisterPlaneDriverEx(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== strcmp @ 0001303c

/* WARNING: Control flow encountered bad instruction data */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int strcmp(char *__s1,char *__s2)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== CamOsMemAlloc @ 00013040

/* WARNING: Control flow encountered bad instruction data */

void CamOsMemAlloc(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== DrvRegisterSensorI2CSlaveID @ 00013044

/* WARNING: Control flow encountered bad instruction data */

void DrvRegisterSensorI2CSlaveID(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== DrvRegisterPlaneI2CSlaveID @ 00013048

/* WARNING: Control flow encountered bad instruction data */

void DrvRegisterPlaneI2CSlaveID(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== DrvSensorHandleVer @ 0001304c

/* WARNING: Control flow encountered bad instruction data */

void DrvSensorHandleVer(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== CamOsUsDelay @ 00013050

/* WARNING: Control flow encountered bad instruction data */

void CamOsUsDelay(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


//==== CamOsMemRelease @ 0001305c

/* WARNING: Control flow encountered bad instruction data */

void CamOsMemRelease(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}


