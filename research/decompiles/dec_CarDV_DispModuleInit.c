
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* CarDV_DispModuleInit() */

int CarDV_DispModuleInit(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  undefined1 *puVar6;
  undefined1 *puVar7;
  undefined1 *puVar8;
  int iVar9;
  bool bVar10;
  undefined4 local_7c;
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  uint local_68;
  uint local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 uStack_58;
  undefined4 local_54;
  undefined1 auStack_50 [44];
  
  iVar1 = MI_DISP_InitDev(0);
  if (iVar1 != 0) {
    printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x18b,iVar1);
    return iVar1;
  }
  puVar7 = gstDispModule;
  iVar1 = 0;
  if (gstDispModule._0_2_ == 0x100) goto LAB_0003bbe4;
  do {
    do {
      puVar7 = puVar7 + 0x734;
      if (iVar1 != 0) {
        iVar2 = 0;
        if (gstDispModule._3688_2_ == 0x100) {
          local_6c = gstDispModule._3696_4_;
          local_70 = gstDispModule._3692_4_;
          iVar2 = MI_DISP_SetWBCSource(0,&local_70);
          if (iVar2 == 0) {
            local_5c = gstDispModule._3700_4_;
            uStack_58 = gstDispModule._3704_4_;
            local_54 = 0xb;
            iVar2 = MI_DISP_SetWBCAttr(0,&local_5c);
            if (iVar2 == 0) {
              iVar2 = MI_DISP_EnableWBC();
              if (iVar2 == 0) {
                gstDispModule[0xe68] = (char)iVar1;
              }
              else {
                printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1ef,iVar2);
              }
            }
            else {
              printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1ee,iVar2);
            }
          }
          else {
            printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1e9,iVar2);
          }
        }
        return iVar2;
      }
      iVar1 = 1;
    } while (gstDispModule._1844_2_ != 0x100);
LAB_0003bbe4:
    memset(auStack_50,0,0x28);
    uStack_58 = *(undefined4 *)(puVar7 + 4);
    local_54 = *(undefined4 *)(puVar7 + 8);
    local_5c = 0x800080;
    iVar2 = MI_DISP_SetPubAttr(iVar1,&local_5c);
    if (iVar2 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x199,iVar2);
      return iVar2;
    }
    iVar2 = MI_DISP_Enable(iVar1);
    if (iVar2 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x19a,iVar2);
      return iVar2;
    }
    uVar5 = *(uint *)(puVar7 + 4);
    *puVar7 = 1;
    bVar10 = 5 < uVar5;
    if (uVar5 != 6) {
      bVar10 = 4 < uVar5 - 9;
    }
    if (!bVar10 || (uVar5 == 6 || uVar5 - 9 == 5)) {
      iVar2 = CarDV_PanelModuleInit();
      if (iVar2 != 0) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1a4,iVar2);
        return iVar2;
      }
      uVar5 = *(uint *)(puVar7 + 4);
    }
    if (uVar5 == 2 || uVar5 == 5) {
      CarDV_DispToHdmiTiming(*(undefined4 *)(puVar7 + 8));
      iVar2 = CarDV_HdmiModuleInit();
      if (iVar2 != 0) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1aa,iVar2);
        return iVar2;
      }
    }
    iVar2 = 0;
    puVar8 = puVar7;
    while( true ) {
      if (*(short *)(gstDispModule + iVar2 * 0x394 + iVar1 * 0x734 + 0xc) == 0x100) {
        local_6c = *(undefined4 *)(puVar8 + 0x1c);
        local_70 = *(undefined4 *)(puVar8 + 0x18);
        local_60 = 0;
        local_68 = (uint)*(ushort *)(puVar8 + 0xe);
        local_64 = (uint)*(ushort *)(puVar8 + 0x10);
        iVar3 = MI_DISP_BindVideoLayer(*(undefined4 *)(puVar8 + 0x14),iVar1);
        if (iVar3 != 0) {
          printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1bc,iVar3);
          return iVar3;
        }
        iVar3 = MI_DISP_SetVideoLayerAttr(*(undefined4 *)(puVar8 + 0x14),&local_70);
        if (iVar3 != 0) {
          printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1bd,iVar3);
          return iVar3;
        }
        iVar3 = MI_DISP_EnableVideoLayer(*(undefined4 *)(puVar8 + 0x14));
        if (iVar3 != 0) {
          printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1be,iVar3);
          return iVar3;
        }
        if (iVar2 == 0) {
          iVar3 = 0x10;
        }
        else {
          iVar3 = 1;
        }
        puVar8[0xc] = 1;
        iVar9 = 0;
        puVar6 = puVar8;
        do {
          if (*(short *)(gstDispModule + iVar9 * 0x38 + iVar1 * 0x734 + iVar2 * 0x394 + 0x20) ==
              0x100) {
            local_78 = *(undefined4 *)(puVar6 + 0x2c);
            local_74 = *(undefined4 *)(puVar6 + 0x24);
            local_7c = *(undefined4 *)(puVar6 + 0x28);
            iVar4 = MI_DISP_SetInputPortAttr(*(undefined4 *)(puVar8 + 0x14),iVar9,&local_7c);
            if (iVar4 != 0) {
              printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1d0,iVar4);
              return iVar4;
            }
            puVar6[0x20] = 1;
            if (((*(short *)(puVar6 + 0x34) != 0) && (*(short *)(puVar6 + 0x36) != 0)) &&
               (iVar4 = MI_DISP_SetZoomInWindow
                                  (*(undefined4 *)(puVar8 + 0x14),iVar9,puVar8 + iVar9 * 0x38 + 0x30
                                  ), iVar4 != 0)) {
              printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleInit",0x1d6,iVar4);
              return iVar4;
            }
          }
          iVar9 = iVar9 + 1;
          puVar6 = puVar6 + 0x38;
        } while (iVar3 != iVar9);
      }
      puVar8 = puVar8 + 0x394;
      if (iVar2 == 1) break;
      iVar2 = 1;
    }
  } while( true );
}

