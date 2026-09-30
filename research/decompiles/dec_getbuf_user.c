
/* WARNING: Removing unreachable block (ram,0x00051176) */

void FUN_000510c4(undefined1 *param_1)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  bool bVar9;
  undefined4 local_1c0;
  int local_1bc;
  int local_1b8;
  int iStack_1b4;
  fd_set fStack_1b0;
  uint local_130;
  int iStack_12c;
  int local_108;
  ushort local_f8;
  ushort local_f6;
  undefined4 local_f4;
  undefined4 local_c4;
  
  uVar4 = *(uint *)(param_1 + 0x1c);
  uVar5 = uVar4 & 0x1f;
  uVar6 = uVar4 + 0x1f;
LAB_000510ec:
  do {
    while( true ) {
      memset(&fStack_1b0,0,0x80);
      uVar1 = *(uint *)(param_1 + 0x24);
      uVar3 = uVar1 & 0x1f;
      uVar8 = uVar1 + 0x1f;
      if (-1 < (int)uVar1) {
        uVar8 = uVar1;
      }
      if (-1 < (int)-uVar1) {
        uVar3 = -(-uVar1 & 0x1f);
      }
      uVar7 = uVar4;
      if ((int)uVar4 < 0) {
        uVar7 = uVar6;
      }
      if (-1 < (int)-uVar4) {
        uVar5 = -(-uVar4 & 0x1f);
      }
      fStack_1b0.fds_bits[(int)uVar8 >> 5] =
           1 << (uVar3 & 0xff) | fStack_1b0.fds_bits[(int)uVar8 >> 5];
      if ((int)uVar1 < (int)uVar4) {
        uVar1 = uVar4;
      }
      fStack_1b0.fds_bits[(int)uVar7 >> 5] =
           1 << (uVar5 & 0xff) | fStack_1b0.fds_bits[(int)uVar7 >> 5];
      iVar2 = select(uVar1 + 1,&fStack_1b0,(fd_set *)0x0,(fd_set *)0x0,(timeval *)0x0);
      if (0 < iVar2) break;
      uVar4 = *(uint *)(param_1 + 0x1c);
      uVar6 = uVar4 + 0x1f;
      uVar5 = uVar4 & 0x1f;
    }
    uVar4 = *(uint *)(param_1 + 0x24);
    uVar5 = uVar4 & 0x1f;
    iVar2 = (int)(uVar4 + 0x1f & (int)uVar4 >> 0x20) >> 5;
    if (-1 < (int)-uVar4) {
      uVar5 = -(-uVar4 & 0x1f);
    }
    uVar5 = 1 << (uVar5 & 0xff);
    uVar4 = fStack_1b0.fds_bits[iVar2];
    if ((uVar4 & uVar5) == 0) {
      uVar4 = *(uint *)(param_1 + 0x1c);
      uVar6 = uVar4 + 0x1f;
      uVar5 = uVar4 & 0x1f;
      uVar3 = uVar4 & ~((int)uVar4 >> 0x20);
      if ((int)uVar4 < 0) {
        uVar3 = uVar6;
      }
      uVar1 = uVar5;
      if (-1 < (int)-uVar4) {
        uVar1 = -(-uVar4 & 0x1f);
      }
    }
    else {
      fStack_1b0.fds_bits[iVar2] = uVar4 & ~uVar5;
      iVar2 = MI_SYS_ChnOutputPortGetBuf(param_1 + 4,&local_130,&local_1c0);
      if (iVar2 == 0) {
        if (DAT_0008ba9c == (code *)0x0) {
LAB_00051216:
          MI_SYS_SetChnOutputPortDepth((undefined2)g_s32SocId,param_1 + 4,0,4);
        }
        else {
          if ((param_1[0x28] == '\0') || (param_1[0x29] != '\0')) {
            if (local_108 == 4) {
              local_1b8 = 0;
              iStack_1b4 = 0;
              local_1bc = 0;
              iVar2 = (uint)local_f6 * (uint)local_f8 * 2;
              MI_SYS_MMA_Alloc((undefined2)g_s32SocId,&DAT_00064320,iVar2,&local_1b8);
              if (local_1b8 == 0 && iStack_1b4 == 0) {
                if (local_1bc == 0) goto LAB_00051216;
LAB_00051304:
                FBD_Execute(local_f4,local_1bc,local_f8,local_f6);
                (*DAT_0008ba9c)(*param_1,local_1bc,iVar2);
                MI_SYS_Munmap(local_1bc,iVar2);
              }
              else {
                MI_SYS_Mmap(local_1b8,iStack_1b4,iVar2,&local_1bc,1);
                if (local_1bc != 0) goto LAB_00051304;
              }
              if (local_1b8 != 0 || iStack_1b4 != 0) {
                MI_SYS_MMA_Free((undefined2)g_s32SocId);
              }
            }
            else {
              (*DAT_0008ba9c)(*param_1,local_f4,local_c4);
            }
            goto LAB_00051216;
          }
          uVar5 = *(uint *)(param_1 + 0x30);
          if (uVar5 == 0 && *(int *)(param_1 + 0x34) == 0) {
            *(uint *)(param_1 + 0x30) = local_130;
            *(int *)(param_1 + 0x34) = iStack_12c;
            if (g_VideoSetExpoCallback == (code *)0x0) goto LAB_00051292;
            (*g_VideoSetExpoCallback)(*param_1,1,*(undefined4 *)(param_1 + 0x38));
          }
          else {
            uVar4 = iStack_12c - *(int *)(param_1 + 0x34);
            bVar9 = uVar4 == local_130 < uVar5;
            if (uVar4 == local_130 < uVar5) {
              bVar9 = local_130 - uVar5 < 500000;
            }
            if (bVar9) {
              *(uint *)(param_1 + 0x30) = local_130;
              *(int *)(param_1 + 0x34) = iStack_12c;
            }
            else {
LAB_00051292:
              param_1[0x29] = 1;
            }
          }
        }
        MI_SYS_ChnOutputPortPutBuf(local_1c0);
        uVar4 = *(uint *)(param_1 + 0x1c);
        uVar6 = uVar4 + 0x1f;
        uVar5 = uVar4 & 0x1f;
        goto LAB_000510ec;
      }
      uVar4 = *(uint *)(param_1 + 0x1c);
      uVar6 = uVar4 + 0x1f;
      uVar5 = uVar4 & 0x1f;
      uVar3 = uVar4 & ~((int)uVar4 >> 0x20);
      if ((int)uVar4 < 0) {
        uVar3 = uVar6;
      }
      uVar1 = uVar5;
      if (-1 < (int)-uVar4) {
        uVar1 = -(-uVar4 & 0x1f);
      }
      if (iVar2 < 1) goto LAB_000510ec;
    }
    if ((1 << (uVar1 & 0xff) & fStack_1b0.fds_bits[(int)uVar3 >> 5]) != 0) {
                    /* WARNING: Subroutine does not return */
      pthread_exit((void *)0x0);
    }
  } while( true );
}


