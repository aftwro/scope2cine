
/* WARNING: Removing unreachable block (ram,0x0002a0c4) */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* cardvInputMessageLoop() */

undefined4 cardvInputMessageLoop(void)

{
  uint __fd;
  uint __fd_00;
  int iVar1;
  __pid_t _Var2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  fd_set fStack_1b0;
  char local_130 [260];
  
  local_130[0] = '\0';
  local_130[1] = '\0';
  local_130[2] = '\0';
  local_130[3] = '\0';
  memset(local_130 + 4,0,0xfc);
  CarDVInitCmdHandler();
  CarDVAddCmdHandler("ctmwb",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002752c_1);
  CarDVAddCmdHandler("playback",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028668_1);
  CarDVAddCmdHandler("frequency",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028608_1);
  CarDVAddCmdHandler("mirror",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028590_1);
  CarDVAddCmdHandler("seamless",'\x01',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000293cc_1);
  CarDVAddCmdHandler("rec",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000292fc_1);
  CarDVAddCmdHandler("capture",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028c54_1);
  CarDVAddCmdHandler("capraw",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028c2c_1);
  CarDVAddCmdHandler("lock",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028138_1);
  CarDVAddCmdHandler("res",'\x04',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002798c_1);
  CarDVAddCmdHandler("vidres",'\x03',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000298a8_1);
  CarDVAddCmdHandler("capres",'\x03',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000278d0_1);
  CarDVAddCmdHandler("loop",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00029614_1);
  CarDVAddCmdHandler("timelapse",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000296b0_1);
  CarDVAddCmdHandler("slowmotion",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)0x294ed);
  CarDVAddCmdHandler("rm",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002856c_1);
  CarDVAddCmdHandler("format",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027834_1);
  CarDVAddCmdHandler("sdrename",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027828_1);
  CarDVAddCmdHandler("thumb",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027e20_1);
  CarDVAddCmdHandler("tt",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027dd4_1);
  CarDVAddCmdHandler("bri",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028550_1);
  CarDVAddCmdHandler("con",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028534_1);
  CarDVAddCmdHandler("sat",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000284ec_1);
  CarDVAddCmdHandler("sha",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000284d0_1);
  CarDVAddCmdHandler("wb",'\x05',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002844c_1);
  CarDVAddCmdHandler("scene",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027da8_1);
  CarDVAddCmdHandler("ev",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027d7c_1);
  CarDVAddCmdHandler("flicker",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027d50_1);
  CarDVAddCmdHandler("shutter",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027d24_1);
  CarDVAddCmdHandler("iso",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028410_1);
  CarDVAddCmdHandler("gamma",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027cf8_1);
  CarDVAddCmdHandler("effect",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027ccc_1);
  CarDVAddCmdHandler("winweight",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027ca0_1);
  CarDVAddCmdHandler("night",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027c74_1);
  CarDVAddCmdHandler("audioplay",'\x04',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027c08_1);
  CarDVAddCmdHandler("aplaystop",'\x03',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027bb4_1);
  CarDVAddCmdHandler("audiorec",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028228_1);
  CarDVAddCmdHandler("pbvolume",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000281e8_1);
  CarDVAddCmdHandler("park",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000283e4_1);
  CarDVAddCmdHandler("gsensor",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000283b8_1);
  CarDVAddCmdHandler("zoom",'\x04',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028378_1);
  CarDVAddCmdHandler("video",'\x03',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000290a0_1);
  CarDVAddCmdHandler("hdmiset",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028324_1);
  CarDVAddCmdHandler("voice",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028188_1);
  CarDVAddCmdHandler("flip",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000273a5);
  CarDVAddCmdHandler("bitrate",'\x03',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000287c8_1);
  CarDVAddCmdHandler("codec",'\x03',(_func_void_uchar_uchar_char_ptr_ptr *)0x27b59);
  CarDVAddCmdHandler("hdmitiming",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027b0c_1);
  CarDVAddCmdHandler("disp",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)0x27669);
  CarDVAddCmdHandler("muxdebug",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027ae4_1);
  CarDVAddCmdHandler("dcf",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000282fc_1);
  CarDVAddCmdHandler("srcchg",'\x03',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00029068_1);
  CarDVAddCmdHandler("timezone",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000282c4_1);
  CarDVAddCmdHandler("quality",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_00027409);
  CarDVAddCmdHandler("micsen",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000273fd);
  CarDVAddCmdHandler("prerec",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000273f1);
  CarDVAddCmdHandler("vmdrectime",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000273e5);
  CarDVAddCmdHandler("autorec",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000273d9);
  CarDVAddCmdHandler("hdr",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002943c_1);
  CarDVAddCmdHandler("wnr",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000273cd);
  CarDVAddCmdHandler("snr",'\x03',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00029474_1);
  CarDVAddCmdHandler("burstshot",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000273c1);
  CarDVAddCmdHandler("dumpimpl",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_00027415);
  CarDVAddCmdHandler("quit",'\x01',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000273a8_1);
  CarDVAddCmdHandler("help",'\x01',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028a5c_1);
  CarDVAddCmdHandler("datelogoStamp",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002810c_1);
  CarDVAddCmdHandler("timeformat",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000280e0_1);
  CarDVAddCmdHandler("str",'\x01',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002a878_1);
  CarDVAddCmdHandler("reset",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002c94c_1);
  CarDVAddCmdHandler("lcdbri",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_000280b4_1);
  CarDVAddCmdHandler("lcdcon",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028088_1);
  CarDVAddCmdHandler("lcdhue",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002805c_1);
  CarDVAddCmdHandler("lcdsat",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028030_1);
  CarDVAddCmdHandler("recvol",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027558_1);
  CarDVAddCmdHandler("sclrotate",'\x04',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00028274_1);
  CarDVAddCmdHandler("aiattr",'\x04',(_func_void_uchar_uchar_char_ptr_ptr *)0x27e6d);
  CarDVAddCmdHandler("player",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)0x27779);
  CarDVAddCmdHandler("ispget",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_00027664_1);
  CarDVAddCmdHandler("ispset",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000287c5);
  CarDVAddCmdHandler("decrypt",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002760c_1);
  CarDVAddCmdHandler("yuvdump",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&DAT_000287c1);
  CarDVAddCmdHandler("scl",'\x06',(_func_void_uchar_uchar_char_ptr_ptr *)0x27581);
  CarDVAddCmdHandler("hdmi",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002cea8_1);
  CarDVAddCmdHandler("dispscal",'\x02',(_func_void_uchar_uchar_char_ptr_ptr *)&LAB_0002ce50_1);
  puts("cardv Loop begin");
  __fd = open("/tmp/cardv_fifo",2);
  if (__fd == 0xffffffff) {
    _Var2 = getpid();
    printf("Process %d open fifo error\n",_Var2);
  }
  else {
    __fd_00 = open("/tmp/cardv_internal",2);
    if (__fd_00 == 0xffffffff) {
      _Var2 = getpid();
      printf("Process %d open fifo high error\n",_Var2);
      close(__fd);
    }
    else {
      uVar3 = __fd;
      if ((int)__fd < (int)__fd_00) {
        uVar3 = __fd_00;
      }
      if (bMessageLoop != 0) {
        uVar8 = __fd & 0x1f;
        if (-1 < (int)-__fd) {
          uVar8 = -(-__fd & 0x1f);
        }
        uVar5 = __fd_00 + 0x1f & (int)__fd_00 >> 0x20;
        if (__fd != 0) {
          uVar5 = __fd_00;
        }
        uVar4 = __fd_00 & 0x1f;
        if (-1 < (int)-__fd_00) {
          uVar4 = -(-__fd_00 & 0x1f);
        }
        uVar8 = 1 << (uVar8 & 0xff);
        uVar4 = 1 << (uVar4 & 0xff);
        iVar7 = (int)(__fd + 0x1f & (int)__fd >> 0x20) >> 5;
        iVar6 = (int)uVar5 >> 5;
        do {
          memset(&fStack_1b0,0,0x80);
          fStack_1b0.fds_bits[iVar7] = fStack_1b0.fds_bits[iVar7] | uVar8;
          fStack_1b0.fds_bits[iVar6] = fStack_1b0.fds_bits[iVar6] | uVar4;
          iVar1 = select(uVar3 + 1,&fStack_1b0,(fd_set *)0x0,(fd_set *)0x0,(timeval *)0x0);
          if (iVar1 < 1) {
            if (iVar1 == 0) {
              puts("FIFO select timeout");
            }
            else {
              puts("FIFO select failed");
            }
            break;
          }
          uVar5 = fStack_1b0.fds_bits[iVar6];
          if ((uVar4 & uVar5) != 0) {
            uVar5 = read(__fd_00,local_130,0x100);
            if (0 < (int)uVar5) {
              cardvInputMessageProcess(local_130,uVar5,true);
              memset(local_130,0,0x100);
            }
            uVar5 = fStack_1b0.fds_bits[iVar6];
          }
          fStack_1b0.fds_bits[iVar6] = uVar5 & ~uVar4;
          uVar5 = fStack_1b0.fds_bits[iVar7];
          if ((uVar8 & uVar5) != 0) {
            uVar5 = read(__fd,local_130,0x100);
            if (0 < (int)uVar5) {
              cardvInputMessageProcess(local_130,uVar5,false);
              memset(local_130,0,0x100);
            }
            uVar5 = fStack_1b0.fds_bits[iVar7];
          }
          fStack_1b0.fds_bits[iVar7] = uVar5 & ~uVar8;
        } while (bMessageLoop != 0);
      }
      close(__fd);
      close(__fd_00);
    }
  }
  CarDVRemoveCmdHandler();
  puts("cardv Loop exit");
  return 0;
}

