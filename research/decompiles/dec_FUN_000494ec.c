
undefined4 FUN_000494ec(int param_1,undefined4 *param_2,size_t *param_3)

{
  undefined1 *puVar1;
  int iVar2;
  size_t sVar3;
  void *__dest;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  timeval local_298;
  undefined1 auStack_290 [16];
  uint local_280;
  undefined1 *local_268;
  uint local_264;
  undefined1 auStack_260 [64];
  fd_set fStack_220;
  undefined1 auStack_1a0 [372];
  
  iVar9 = param_1 + 0x14;
  uVar6 = *(uint *)(&DAT_0008b7ac + param_1 * 4);
  uVar8 = uVar6 + 0x1f;
  uVar7 = uVar6 & 0x1f;
  while( true ) {
    while( true ) {
      do {
        while( true ) {
          memset(&fStack_220,0,0x80);
          uVar5 = uVar6 & ~((int)uVar6 >> 0x20);
          if ((int)uVar6 < 0) {
            uVar5 = uVar8;
          }
          if (-1 < (int)-uVar6) {
            uVar7 = -(-uVar6 & 0x1f);
          }
          fStack_220.fds_bits[(int)uVar5 >> 5] =
               1 << (uVar7 & 0xff) | fStack_220.fds_bits[(int)uVar5 >> 5];
          local_298.tv_sec = 0;
          local_298.tv_usec = 500000;
          iVar2 = select(uVar6 + 1,&fStack_220,(fd_set *)0x0,(fd_set *)0x0,&local_298);
          if (0 < iVar2) break;
          uVar6 = *(uint *)(&DAT_0008b7ac + param_1 * 4);
          uVar8 = uVar6 + 0x1f;
          uVar7 = uVar6 & 0x1f;
        }
        uVar6 = *(uint *)(&DAT_0008b7ac + param_1 * 4);
        uVar8 = uVar6 + 0x1f;
        uVar7 = uVar6 & 0x1f;
        uVar5 = uVar6 & ~((int)uVar6 >> 0x20);
        if ((int)uVar6 < 0) {
          uVar5 = uVar8;
        }
        uVar4 = uVar7;
        if (-1 < (int)-uVar6) {
          uVar4 = -(-uVar6 & 0x1f);
        }
      } while ((1 << (uVar4 & 0xff) & fStack_220.fds_bits[(int)uVar5 >> 5]) == 0);
      memset(auStack_290,0,0x28);
      iVar2 = MI_VENC_Query(8,iVar9,auStack_290);
      uVar7 = local_280;
      if ((iVar2 == 0) && (local_280 != 0)) break;
      if (g_dbglevel < 6) {
        printf("\x1b[0;31m[ERR]:%s[%d]: \x1b[0m","_GetJpegThumbnail",0x1f0);
        printf("JpegThumb Chn%d Query Error\n",iVar9);
        uVar6 = *(uint *)(&DAT_0008b7ac + param_1 * 4);
        uVar8 = uVar6 + 0x1f;
        uVar7 = uVar6 & 0x1f;
      }
      else {
        uVar6 = *(uint *)(&DAT_0008b7ac + param_1 * 4);
        uVar8 = uVar6 + 0x1f;
        uVar7 = uVar6 & 0x1f;
      }
    }
    memset(auStack_260,0,0x40);
    memset(auStack_1a0,0,0x170);
    local_264 = uVar7;
    local_268 = auStack_1a0;
    if ((2 < uVar7) && (g_dbglevel < 6)) {
      printf("\x1b[0;31m[ERR]:%s[%d]: \x1b[0m","_GetJpegThumbnail",0x1f9);
      printf("CurPacks [%d] > STREAM_PACK_CNT \n",local_280);
    }
    iVar2 = MI_VENC_GetStream(8,iVar9,&local_268,0xffffffff);
    puVar1 = local_268;
    if (iVar2 == 0) break;
    uVar6 = *(uint *)(&DAT_0008b7ac + param_1 * 4);
    uVar8 = uVar6 + 0x1f;
    uVar7 = uVar6 & 0x1f;
  }
  *param_3 = 0;
  sVar3 = *(size_t *)(local_268 + 0xc);
  *param_3 = sVar3;
  sVar3 = sVar3 + *(int *)(local_268 + 0xc4);
  *param_3 = sVar3;
  __dest = malloc(sVar3);
  *param_2 = __dest;
  if ((__dest != (void *)0x0) && (*(size_t *)(puVar1 + 0xc) != 0)) {
    memcpy(__dest,*(void **)(puVar1 + 8),*(size_t *)(puVar1 + 0xc));
    if (*(size_t *)(puVar1 + 0xc4) != 0) {
      memcpy((void *)(*(int *)(puVar1 + 0xc) + (int)__dest),*(void **)(puVar1 + 0xc0),
             *(size_t *)(puVar1 + 0xc4));
    }
  }
  MI_VENC_ReleaseStream(8,iVar9,&local_268);
  return 0;
}

