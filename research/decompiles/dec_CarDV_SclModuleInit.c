
/* CarDV_SclModuleInit(unsigned int) */

uint CarDV_SclModuleInit(uint param_1)

{
  ushort uVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined1 *puVar6;
  undefined1 *puVar7;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  uint local_54;
  int local_50;
  uint local_4c;
  undefined4 local_48;
  undefined4 uStack_44;
  undefined4 local_40;
  uint local_3c;
  undefined4 local_38;
  uint local_34;
  
  iVar4 = param_1 * 0xd90;
  if ((*(uint *)(gstSclModule + iVar4) & 0xffff00) == 0x100) {
    local_48 = *(undefined4 *)(gstSclModule + iVar4 + 4);
    uVar3 = MI_SCL_CreateDevice(param_1,&local_48);
    if (uVar3 != 0) {
      printf("(%s %d)exec func fail, err:%x\n","CarDV_SclModuleInit",0x38,uVar3);
      return uVar3;
    }
    gstSclModule[iVar4 + 2] = 1;
  }
  iVar5 = param_1 * 0xd90;
  iVar4 = 0;
  puVar6 = gstSclModule + iVar5;
  puVar7 = puVar6;
  do {
    if ((*(uint *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0x10) & 0xffff00) == 0x100) {
      local_60 = 0;
      uVar3 = MI_SCL_CreateChannel(param_1,iVar4,&local_60);
      if (uVar3 != 0) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_SclModuleInit",0x43,uVar3);
        return uVar3;
      }
      puVar6[0x12] = 1;
      if (((*(short *)(puVar6 + 0x1c) != 0) && (*(short *)(puVar6 + 0x1e) != 0)) &&
         (uVar3 = MI_SCL_SetInputPortCrop(param_1,iVar4,puVar7 + 0x18), uVar3 != 0)) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_SclModuleInit",0x49,uVar3);
        return uVar3;
      }
      local_5c = *(undefined4 *)(puVar6 + 0x14);
      uVar3 = MI_SCL_SetChnParam(param_1,iVar4,&local_5c);
      if (uVar3 != 0) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_SclModuleInit",0x50,uVar3);
        return uVar3;
      }
      uVar3 = MI_SCL_StartChannel(param_1,iVar4);
      if (uVar3 != 0) {
        printf("(%s %d)exec func fail, err:%x\n","CarDV_SclModuleInit",0x51,uVar3);
        return uVar3;
      }
      if (*(short *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0x40) == 1) {
        local_48 = *(undefined4 *)(puVar7 + 0x50);
        uStack_44 = *(undefined4 *)(puVar7 + 0x54);
        local_40 = *(undefined4 *)(puVar7 + 0x4c);
        local_38 = *(undefined4 *)(puVar6 + 0x48);
        local_3c = (uint)*(ushort *)(puVar6 + 0x43);
        local_34 = uVar3;
        uVar3 = MI_SCL_SetOutputPortParam(param_1,iVar4);
        if (uVar3 != 0) goto LAB_00039c90;
        uVar3 = MI_SCL_EnableOutputPort(param_1,iVar4);
        if (uVar3 != 0) goto LAB_00039ca4;
        puVar6[0x42] = 1;
        local_58 = 0x22;
        local_54 = param_1;
        local_50 = iVar4;
        local_4c = uVar3;
        uVar3 = MI_SYS_SetChnOutputPortDepth
                          (0,&local_58,*(undefined2 *)(puVar6 + 0x5a),*(undefined2 *)(puVar6 + 0x58)
                          );
        if (uVar3 != 0) goto LAB_00039cb8;
        puVar6[0x41] = 1;
        uVar1 = *(ushort *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0x5c);
      }
      else {
        uVar1 = *(ushort *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0x5c);
      }
      if (uVar1 == 1) {
        local_48 = *(undefined4 *)(puVar7 + 0x6c);
        uStack_44 = *(undefined4 *)(puVar7 + 0x70);
        local_40 = *(undefined4 *)(puVar7 + 0x68);
        local_38 = *(undefined4 *)(puVar6 + 100);
        local_34 = 0;
        local_3c = (uint)*(ushort *)(puVar6 + 0x5f);
        uVar3 = MI_SCL_SetOutputPortParam(param_1,iVar4,1);
        if (uVar3 != 0) goto LAB_00039c90;
        uVar3 = MI_SCL_EnableOutputPort(param_1,iVar4,1);
        if (uVar3 != 0) goto LAB_00039ca4;
        puVar6[0x5e] = (char)uVar1;
        local_58 = 0x22;
        local_54 = param_1;
        local_50 = iVar4;
        local_4c = (uint)uVar1;
        uVar3 = MI_SYS_SetChnOutputPortDepth
                          (0,&local_58,*(undefined2 *)(puVar6 + 0x76),*(undefined2 *)(puVar6 + 0x74)
                          );
        if (uVar3 != 0) goto LAB_00039cb8;
        puVar6[0x5d] = (char)uVar1;
        if (*(short *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0x78) == 1) goto LAB_00039ab8;
LAB_00039886:
        if (*(short *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0x94) != 1) goto LAB_00039898;
LAB_00039b5a:
        local_48 = *(undefined4 *)(puVar7 + 0xa4);
        uStack_44 = *(undefined4 *)(puVar7 + 0xa8);
        local_40 = *(undefined4 *)(puVar7 + 0xa0);
        local_38 = *(undefined4 *)(puVar6 + 0x9c);
        local_34 = 0;
        local_3c = (uint)*(ushort *)(puVar6 + 0x97);
        uVar3 = MI_SCL_SetOutputPortParam(param_1,iVar4,3);
        if (uVar3 != 0) goto LAB_00039c90;
        uVar3 = MI_SCL_EnableOutputPort(param_1,iVar4,3);
        if (uVar3 != 0) goto LAB_00039ca4;
        puVar6[0x96] = 1;
        local_58 = 0x22;
        local_4c = 3;
        local_54 = param_1;
        local_50 = iVar4;
        uVar3 = MI_SYS_SetChnOutputPortDepth
                          (0,&local_58,*(undefined2 *)(puVar6 + 0xae),*(undefined2 *)(puVar6 + 0xac)
                          );
        if (uVar3 != 0) goto LAB_00039cb8;
        puVar6[0x95] = 1;
        sVar2 = *(short *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0xb0);
      }
      else {
        if (*(short *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0x78) != 1) goto LAB_00039886;
LAB_00039ab8:
        local_48 = *(undefined4 *)(puVar7 + 0x88);
        uStack_44 = *(undefined4 *)(puVar7 + 0x8c);
        local_40 = *(undefined4 *)(puVar7 + 0x84);
        local_38 = *(undefined4 *)(puVar6 + 0x80);
        local_34 = 0;
        local_3c = (uint)*(ushort *)(puVar6 + 0x7b);
        uVar3 = MI_SCL_SetOutputPortParam(param_1,iVar4,2);
        if (uVar3 != 0) goto LAB_00039c90;
        uVar3 = MI_SCL_EnableOutputPort(param_1,iVar4,2);
        if (uVar3 != 0) goto LAB_00039ca4;
        puVar6[0x7a] = 1;
        local_58 = 0x22;
        local_4c = 2;
        local_54 = param_1;
        local_50 = iVar4;
        uVar3 = MI_SYS_SetChnOutputPortDepth
                          (0,&local_58,*(undefined2 *)(puVar6 + 0x92),*(undefined2 *)(puVar6 + 0x90)
                          );
        if (uVar3 != 0) goto LAB_00039cb8;
        puVar6[0x79] = 1;
        if (*(short *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0x94) == 1) goto LAB_00039b5a;
LAB_00039898:
        sVar2 = *(short *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0xb0);
      }
      if (sVar2 == 1) {
        local_48 = *(undefined4 *)(puVar7 + 0xc0);
        uStack_44 = *(undefined4 *)(puVar7 + 0xc4);
        local_40 = *(undefined4 *)(puVar7 + 0xbc);
        local_38 = *(undefined4 *)(puVar6 + 0xb8);
        local_34 = 0;
        local_3c = (uint)*(ushort *)(puVar6 + 0xb3);
        uVar3 = MI_SCL_SetOutputPortParam(param_1,iVar4,4);
        if (uVar3 != 0) goto LAB_00039c90;
        uVar3 = MI_SCL_EnableOutputPort(param_1,iVar4,4);
        if (uVar3 != 0) goto LAB_00039ca4;
        puVar6[0xb2] = 1;
        local_58 = 0x22;
        local_4c = 4;
        local_54 = param_1;
        local_50 = iVar4;
        uVar3 = MI_SYS_SetChnOutputPortDepth
                          (0,&local_58,*(undefined2 *)(puVar6 + 0xca),*(undefined2 *)(puVar6 + 200))
        ;
        if (uVar3 != 0) goto LAB_00039cb8;
        puVar6[0xb1] = 1;
      }
      if (*(short *)(gstSclModule + iVar4 * 0xd8 + iVar5 + 0xcc) == 1) {
        local_48 = *(undefined4 *)(puVar7 + 0xdc);
        uStack_44 = *(undefined4 *)(puVar7 + 0xe0);
        local_40 = *(undefined4 *)(puVar7 + 0xd8);
        local_38 = *(undefined4 *)(puVar6 + 0xd4);
        local_34 = 0;
        local_3c = (uint)*(ushort *)(puVar6 + 0xcf);
        uVar3 = MI_SCL_SetOutputPortParam(param_1,iVar4,5);
        if (uVar3 != 0) {
LAB_00039c90:
          printf("(%s %d)exec func fail, err:%x\n","CarDV_SclModuleInit",0x61,uVar3);
          return uVar3;
        }
        uVar3 = MI_SCL_EnableOutputPort(param_1,iVar4,5);
        if (uVar3 != 0) {
LAB_00039ca4:
          printf("(%s %d)exec func fail, err:%x\n","CarDV_SclModuleInit",99,uVar3);
          return uVar3;
        }
        puVar6[0xce] = 1;
        local_58 = 0x22;
        local_4c = 5;
        local_54 = param_1;
        local_50 = iVar4;
        uVar3 = MI_SYS_SetChnOutputPortDepth
                          (0,&local_58,*(undefined2 *)(puVar6 + 0xe6),*(undefined2 *)(puVar6 + 0xe4)
                          );
        if (uVar3 != 0) {
LAB_00039cb8:
          printf("(%s %d)exec func fail, err:%x\n","CarDV_SclModuleInit",0x6b,uVar3);
          return uVar3;
        }
        puVar6[0xcd] = 1;
      }
    }
    puVar7 = puVar7 + 0xd8;
    puVar6 = puVar6 + 0xd8;
    iVar4 = iVar4 + 1;
    if (iVar4 == 0x10) {
      return 0;
    }
  } while( true );
}

