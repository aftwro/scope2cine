
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* CreatePlayerDev() */

undefined4 CreatePlayerDev(void)

{
  cardv_send_cmd_HP(0x804,0);
  cardv_send_cmd_HP(0x805,0);
  usleep(300000);
  return 0;
}



/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* CarDV_DispPlayBackPipeBind() */

int CarDV_DispPlayBackPipeBind(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  
  uVar3 = 0;
  uVar2 = gstDispBindAttr._4_4_;
  if (gstDispBindAttr._4_4_ != 0) {
    do {
      while (gstDispBindAttr[uVar3 + 0x358] == '\0') {
        iVar1 = CarDV_Sys_Bind((short)uVar3 * 0x30 + 0x891b8);
        uVar2 = gstDispBindAttr._4_4_;
        if (iVar1 != 0) {
          printf("(%s %d)exec func fail, err:%x\n","CarDV_DispPlayBackPipeBind",0x319,iVar1);
          return iVar1;
        }
        gstDispBindAttr[uVar3 + 0x358] = 1;
        uVar3 = uVar3 + 1 & 0xff;
        if (uVar2 <= uVar3) {
          return 0;
        }
      }
      uVar3 = uVar3 + 1 & 0xff;
    } while (uVar3 < uVar2);
  }
  return 0;
}



/* CarDV_DispModuleBindPreOrPlayer(unsigned char, unsigned char) */

int CarDV_DispModuleBindPreOrPlayer(uchar param_1,uchar param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined1 *puVar6;
  undefined1 *puVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 uStack_4c;
  undefined4 local_48;
  int iStack_44;
  int local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 uStack_34;
  undefined4 local_30;
  
  iVar9 = 0;
  while( true ) {
    iVar4 = iVar9 * 0x734;
    puVar7 = gstDispModule + iVar4;
    if (*(short *)(gstDispModule + iVar4) == 0x101) {
      iVar2 = 0;
      while( true ) {
        if (*(short *)(gstDispModule + iVar2 * 0x394 + iVar4 + 0xc) == 0x101) {
          iVar5 = 0;
          puVar6 = puVar7;
          if (iVar2 == 0) {
            iVar10 = 0x10;
          }
          else {
            iVar10 = 1;
          }
          do {
            if ((*(short *)(gstDispModule + iVar5 * 0x38 + iVar4 + iVar2 * 0x394 + 0x20) == 0x101)
               && (iVar8 = *(int *)(puVar6 + 0x38), iVar8 != 0x27)) {
              memset(&local_54,0,0x2c);
              local_54 = 3;
              if (param_2 == '\0') {
                local_50 = *(undefined4 *)(puVar6 + 0x40);
                uStack_4c = *(undefined4 *)(puVar6 + 0x44);
                local_54 = *(undefined4 *)(puVar6 + 0x3c);
              }
              local_48 = 0xf;
              uVar3 = *(uint *)(puVar7 + 0x14) & 0xfffffffd;
              iVar1 = iVar5;
              local_58 = iVar8;
              iStack_44 = iVar9;
              if ((uVar3 != 0) && ((uVar3 != 1 || (iVar1 = 0x10, iVar5 != 0)))) {
                printf("invalid layer%d port%d\n",*(uint *)(puVar7 + 0x14),iVar5);
                iVar1 = 0;
              }
              local_30 = *(undefined4 *)(puVar6 + 0x48);
              local_38 = *(undefined4 *)(puVar6 + 0x50);
              uStack_34 = *(undefined4 *)(puVar6 + 0x54);
              local_3c = 0;
              local_40 = iVar1;
              if (param_1 == '\0') {
                if (puVar6[0x22] != '\0') {
                  iVar8 = CarDV_Sys_UnBind(&local_58);
                  if (iVar8 != 0) {
                    printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleBindPreOrPlayer",0x2ac
                           ,iVar8);
                    return iVar8;
                  }
                  puVar6[0x22] = 0;
                }
              }
              else if (puVar6[0x22] == '\0') {
                iVar8 = CarDV_Sys_Bind(&local_58);
                if (iVar8 != 0) {
                  printf("(%s %d)exec func fail, err:%x\n","CarDV_DispModuleBindPreOrPlayer",0x2a7,
                         iVar8);
                  return iVar8;
                }
                puVar6[0x22] = 1;
              }
            }
            iVar5 = iVar5 + 1;
            puVar6 = puVar6 + 0x38;
          } while (iVar10 != iVar5);
        }
        puVar7 = puVar7 + 0x394;
        if (iVar2 != 0) break;
        iVar2 = 1;
      }
    }
    if (iVar9 != 0) break;
    iVar9 = 1;
  }
  return 0;
}


