===== FUN_0002ea04-0002ea04

void FUN_0002ea04(int *param_1)

{
  short sVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  short sVar6;
  short sVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  ushort *puVar12;
  int iVar13;
  int iVar14;
  ushort *puVar15;
  int iVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  int iVar20;
  int iVar21;
  int iVar22;
  int iVar23;
  int iVar24;
  int iVar25;
  int iVar26;
  int iVar27;
  int iVar28;
  ushort *in_stack_00000000;
  int in_stack_00000004;
  
  iVar14 = ((*param_1 >> 0x10) + 0x20) * 0x3fff;
  iVar13 = ((short)*param_1 + 0x20) * 0x3fff;
  iVar9 = (int)(short)param_1[0x10];
  iVar11 = (int)(short)((uint)param_1[0x10] >> 0x10);
  sVar4 = (short)param_1[8];
  sVar6 = (short)((uint)param_1[8] >> 0x10);
  sVar8 = (short)param_1[0x18];
  iVar10 = iVar13 + iVar9 * 0x3fff + sVar4 * 0x539f + sVar8 * 0x22a3;
  iVar24 = iVar13 + iVar9 * 0x3fff + sVar4 * -0x539f + sVar8 * -0x22a3;
  iVar16 = iVar13 + iVar9 * -0x3fff + sVar4 * 0x22a3 + sVar8 * -0x539f;
  iVar20 = iVar13 + iVar9 * -0x3fff + sVar4 * -0x22a3 + sVar8 * 0x539f;
  sVar4 = (short)((uint)param_1[0x18] >> 0x10);
  iVar13 = iVar14 + iVar11 * 0x3fff + sVar6 * 0x539f + sVar4 * 0x22a3;
  iVar26 = iVar14 + iVar11 * 0x3fff + sVar6 * -0x539f + sVar4 * -0x22a3;
  iVar18 = iVar14 + iVar11 * -0x3fff + sVar6 * 0x22a3 + sVar4 * -0x539f;
  iVar22 = iVar14 + iVar11 * -0x3fff + sVar6 * -0x22a3 + sVar4 * 0x539f;
  sVar5 = (short)param_1[4];
  sVar7 = (short)((uint)param_1[4] >> 0x10);
  sVar4 = (short)param_1[0xc];
  sVar1 = (short)((uint)param_1[0xc] >> 0x10);
  sVar6 = (short)param_1[0x14];
  sVar2 = (short)((uint)param_1[0x14] >> 0x10);
  sVar8 = (short)param_1[0x1c];
  iVar11 = sVar8 * 0x11a8 + sVar6 * 0x3249 + sVar4 * 0x4b42 + sVar5 * 0x58c5;
  iVar17 = sVar8 * 0x3249 + sVar6 * 0x58c5 + sVar4 * 0x11a8 + sVar5 * -0x4b42;
  iVar21 = sVar8 * 0x4b42 + sVar6 * 0x11a8 + sVar5 * 0x3249 + sVar4 * -0x58c5;
  sVar3 = (short)((uint)param_1[0x1c] >> 0x10);
  iVar14 = sVar3 * 0x11a8 + sVar2 * 0x3249 + sVar1 * 0x4b42 + sVar7 * 0x58c5;
  iVar25 = sVar6 * 0x4b42 + sVar5 * 0x11a8 + sVar4 * -0x3249 + sVar8 * -0x58c5;
  iVar19 = sVar3 * 0x3249 + sVar2 * 0x58c5 + sVar1 * 0x11a8 + sVar7 * -0x4b42;
  iVar23 = sVar3 * 0x4b42 + sVar2 * 0x11a8 + sVar7 * 0x3249 + sVar1 * -0x58c5;
  iVar27 = sVar2 * 0x4b42 + sVar7 * 0x11a8 + sVar1 * -0x3249 + sVar3 * -0x58c5;
  iVar9 = iVar10 + iVar11 >> 0x14;
  if (iVar9 < 0) {
    iVar9 = 0;
  }
  if (0xff < iVar9) {
    iVar9 = 0xff;
  }
  iVar28 = iVar13 + iVar14 >> 0x14;
  if (iVar28 < 0) {
    iVar28 = 0;
  }
  if (0xff < iVar28) {
    iVar28 = 0xff;
  }
  iVar11 = iVar10 - iVar11 >> 0x14;
  if (iVar11 < 0) {
    iVar11 = 0;
  }
  if (0xff < iVar11) {
    iVar11 = 0xff;
  }
  iVar14 = iVar13 - iVar14 >> 0x14;
  if (iVar14 < 0) {
    iVar14 = 0;
  }
  if (0xff < iVar14) {
    iVar14 = 0xff;
  }
  *in_stack_00000000 = (ushort)iVar9 | (ushort)(iVar28 << 8);
  puVar15 = (ushort *)(in_stack_00000004 * 7 + (int)in_stack_00000000);
  *puVar15 = (ushort)iVar11 | (ushort)(iVar14 << 8);
  iVar9 = iVar16 - iVar17 >> 0x14;
  if (iVar9 < 0) {
    iVar9 = 0;
  }
  if (0xff < iVar9) {
    iVar9 = 0xff;
  }
  iVar11 = iVar18 - iVar19 >> 0x14;
  if (iVar11 < 0) {
    iVar11 = 0;
  }
  if (0xff < iVar11) {
    iVar11 = 0xff;
  }
  *(ushort *)((int)in_stack_00000000 + in_stack_00000004) = (ushort)iVar9 | (ushort)(iVar11 << 8);
  iVar9 = iVar16 + iVar17 >> 0x14;
  if (iVar9 < 0) {
    iVar9 = 0;
  }
  if (0xff < iVar9) {
    iVar9 = 0xff;
  }
  iVar11 = iVar18 + iVar19 >> 0x14;
  if (iVar11 < 0) {
    iVar11 = 0;
  }
  if (0xff < iVar11) {
    iVar11 = 0xff;
  }
  puVar15 = (ushort *)((int)puVar15 - in_stack_00000004);
  *puVar15 = (ushort)iVar9 | (ushort)(iVar11 << 8);
  iVar9 = iVar20 + iVar21 >> 0x14;
  if (iVar9 < 0) {
    iVar9 = 0;
  }
  if (0xff < iVar9) {
    iVar9 = 0xff;
  }
  iVar11 = iVar22 + iVar23 >> 0x14;
  if (iVar11 < 0) {
    iVar11 = 0;
  }
  if (0xff < iVar11) {
    iVar11 = 0xff;
  }
  puVar12 = (ushort *)((int)in_stack_00000000 + in_stack_00000004 + in_stack_00000004);
  *puVar12 = (ushort)iVar9 | (ushort)(iVar11 << 8);
  iVar9 = iVar20 - iVar21 >> 0x14;
  if (iVar9 < 0) {
    iVar9 = 0;
  }
  if (0xff < iVar9) {
    iVar9 = 0xff;
  }
  iVar11 = iVar22 - iVar23 >> 0x14;
  if (iVar11 < 0) {
    iVar11 = 0;
  }
  if (0xff < iVar11) {
    iVar11 = 0xff;
  }
  puVar15 = (ushort *)((int)puVar15 - in_stack_00000004);
  *puVar15 = (ushort)iVar9 | (ushort)(iVar11 << 8);
  iVar9 = iVar24 + iVar25 >> 0x14;
  if (iVar9 < 0) {
    iVar9 = 0;
  }
  if (0xff < iVar9) {
    iVar9 = 0xff;
  }
  iVar11 = iVar26 + iVar27 >> 0x14;
  if (iVar11 < 0) {
    iVar11 = 0;
  }
  if (0xff < iVar11) {
    iVar11 = 0xff;
  }
  *(ushort *)((int)puVar12 + in_stack_00000004) = (ushort)iVar9 | (ushort)(iVar11 << 8);
  iVar9 = iVar24 - iVar25 >> 0x14;
  if (iVar9 < 0) {
    iVar9 = 0;
  }
  if (0xff < iVar9) {
    iVar9 = 0xff;
  }
  iVar11 = iVar26 - iVar27 >> 0x14;
  if (iVar11 < 0) {
    iVar11 = 0;
  }
  if (0xff < iVar11) {
    iVar11 = 0xff;
  }
  *(ushort *)((int)puVar15 - in_stack_00000004) = (ushort)iVar9 | (ushort)(iVar11 << 8);
  return;
}


===== FUN_0002e7c8-0002e7c8

void FUN_0002e7c8(uint *param_1)

{
  short sVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  short sVar6;
  short sVar7;
  short sVar8;
  uint uVar9;
  uint uVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  int iVar20;
  int iVar21;
  int iVar22;
  int iVar23;
  int iVar24;
  int iVar25;
  int iVar26;
  
  iVar16 = (((int)*param_1 >> 0x10) + 0x20) * 0x3fff;
  iVar13 = ((short)*param_1 + 0x20) * 0x3fff;
  iVar12 = (int)(short)param_1[0x10];
  iVar14 = (int)(short)(param_1[0x10] >> 0x10);
  sVar4 = (short)param_1[8];
  sVar6 = (short)(param_1[8] >> 0x10);
  sVar8 = (short)param_1[0x18];
  iVar11 = iVar13 + iVar12 * 0x3fff + sVar4 * 0x539f + sVar8 * 0x22a3;
  iVar23 = iVar13 + iVar12 * 0x3fff + sVar4 * -0x539f + sVar8 * -0x22a3;
  iVar15 = iVar13 + iVar12 * -0x3fff + sVar4 * 0x22a3 + sVar8 * -0x539f;
  iVar19 = iVar13 + iVar12 * -0x3fff + sVar4 * -0x22a3 + sVar8 * 0x539f;
  sVar4 = (short)(param_1[0x18] >> 0x10);
  iVar13 = iVar16 + iVar14 * 0x3fff + sVar6 * 0x539f + sVar4 * 0x22a3;
  iVar25 = iVar16 + iVar14 * 0x3fff + sVar6 * -0x539f + sVar4 * -0x22a3;
  iVar17 = iVar16 + iVar14 * -0x3fff + sVar6 * 0x22a3 + sVar4 * -0x539f;
  iVar21 = iVar16 + iVar14 * -0x3fff + sVar6 * -0x22a3 + sVar4 * 0x539f;
  sVar5 = (short)param_1[4];
  sVar7 = (short)(param_1[4] >> 0x10);
  sVar4 = (short)param_1[0xc];
  sVar1 = (short)(param_1[0xc] >> 0x10);
  sVar6 = (short)param_1[0x14];
  sVar2 = (short)(param_1[0x14] >> 0x10);
  sVar8 = (short)param_1[0x1c];
  iVar12 = sVar8 * 0x11a8 + sVar6 * 0x3249 + sVar4 * 0x4b42 + sVar5 * 0x58c5;
  iVar16 = sVar8 * 0x3249 + sVar6 * 0x58c5 + sVar4 * 0x11a8 + sVar5 * -0x4b42;
  iVar20 = sVar8 * 0x4b42 + sVar6 * 0x11a8 + sVar5 * 0x3249 + sVar4 * -0x58c5;
  sVar3 = (short)(param_1[0x1c] >> 0x10);
  iVar14 = sVar3 * 0x11a8 + sVar2 * 0x3249 + sVar1 * 0x4b42 + sVar7 * 0x58c5;
  iVar24 = sVar6 * 0x4b42 + sVar5 * 0x11a8 + sVar4 * -0x3249 + sVar8 * -0x58c5;
  iVar18 = sVar3 * 0x3249 + sVar2 * 0x58c5 + sVar1 * 0x11a8 + sVar7 * -0x4b42;
  iVar22 = sVar3 * 0x4b42 + sVar2 * 0x11a8 + sVar7 * 0x3249 + sVar1 * -0x58c5;
  iVar26 = sVar2 * 0x4b42 + sVar7 * 0x11a8 + sVar1 * -0x3249 + sVar3 * -0x58c5;
  uVar9 = iVar11 + iVar12;
  uVar10 = uVar9 >> 0x14;
  if ((int)uVar9 < 0) {
    uVar10 = uVar10 | 0xf000;
  }
  *param_1 = uVar10 | (iVar13 + iVar14 >> 0x14) << 0x10;
  uVar10 = iVar11 - iVar12;
  uVar9 = uVar10 >> 0x14;
  if ((int)uVar10 < 0) {
    uVar9 = uVar9 | 0xf000;
  }
  param_1[0x1c] = uVar9 | (iVar13 - iVar14 >> 0x14) << 0x10;
  uVar9 = iVar15 - iVar16;
  uVar10 = uVar9 >> 0x14;
  if ((int)uVar9 < 0) {
    uVar10 = uVar10 | 0xf000;
  }
  param_1[4] = uVar10 | (iVar17 - iVar18 >> 0x14) << 0x10;
  uVar10 = iVar15 + iVar16;
  uVar9 = uVar10 >> 0x14;
  if ((int)uVar10 < 0) {
    uVar9 = uVar9 | 0xf000;
  }
  param_1[0x18] = uVar9 | (iVar17 + iVar18 >> 0x14) << 0x10;
  uVar9 = iVar19 + iVar20;
  uVar10 = uVar9 >> 0x14;
  if ((int)uVar9 < 0) {
    uVar10 = uVar10 | 0xf000;
  }
  param_1[8] = uVar10 | (iVar21 + iVar22 >> 0x14) << 0x10;
  uVar10 = iVar19 - iVar20;
  uVar9 = uVar10 >> 0x14;
  if ((int)uVar10 < 0) {
    uVar9 = uVar9 | 0xf000;
  }
  param_1[0x14] = uVar9 | (iVar21 - iVar22 >> 0x14) << 0x10;
  uVar9 = iVar23 + iVar24;
  uVar10 = uVar9 >> 0x14;
  if ((int)uVar9 < 0) {
    uVar10 = uVar10 | 0xf000;
  }
  param_1[0xc] = uVar10 | (iVar25 + iVar26 >> 0x14) << 0x10;
  uVar10 = iVar23 - iVar24;
  uVar9 = uVar10 >> 0x14;
  if ((int)uVar10 < 0) {
    uVar9 = uVar9 | 0xf000;
  }
  param_1[0x10] = uVar9 | (iVar25 - iVar26 >> 0x14) << 0x10;
  return;
}


===== 0000e2a6

