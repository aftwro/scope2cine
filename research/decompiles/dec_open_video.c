
undefined4 open_video(void *param_1)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  int *piVar4;
  int iVar5;
  
  if (param_1 == (void *)0x0) {
    return 0;
  }
  if (*(int *)((int)param_1 + 0x1c) < 0) {
LAB_0003fc94:
    uVar1 = 0;
  }
  else {
    iVar5 = *(int *)(*(int *)((int)param_1 + 0xc) + 0xb0);
    iVar2 = *(int *)(iVar5 + 4);
    if (iVar2 == 0x1b) {
      puVar3 = (undefined4 *)avcodec_find_decoder_by_name("ssh264");
      *(undefined4 *)((int)param_1 + 0xb8c) = 1;
    }
    else if (iVar2 == 0xad) {
      puVar3 = (undefined4 *)avcodec_find_decoder_by_name("sshevc");
      *(undefined4 *)((int)param_1 + 0xb8c) = 1;
    }
    else if (iVar2 == 7) {
      puVar3 = (undefined4 *)avcodec_find_decoder_by_name("ssmjpeg");
      *(undefined4 *)((int)param_1 + 0xb8c) = 1;
    }
    else {
      puVar3 = (undefined4 *)avcodec_find_decoder();
      *(undefined4 *)((int)param_1 + 0xb8c) = 1;
    }
    if (puVar3 == (undefined4 *)0x0) {
      puts("Can\'t find codec!");
    }
    else {
      printf("open codec [%s]\n",*puVar3);
      iVar2 = avcodec_alloc_context3(puVar3);
      if (iVar2 == 0) {
        puts("avcodec_alloc_context3() failed");
      }
      else {
        iVar5 = avcodec_parameters_to_context(iVar2,iVar5);
        if (iVar5 < 0) {
          puts("avcodec_parameters_to_context() failed");
        }
        else {
          iVar5 = avcodec_open2(iVar2,puVar3,0);
          if (-1 < iVar5) {
            *(int *)((int)param_1 + 0x14) = iVar2;
            *(undefined4 *)(iVar2 + 0x240) = 1;
            *(undefined4 *)(iVar2 + 0x3c) = 0;
            iVar2 = pthread_create((pthread_t *)((int)param_1 + 0xb84),(pthread_attr_t *)0x0,
                                   (__start_routine *)0x3f501,param_1);
            if (iVar2 != 0) {
              piVar4 = __errno_location();
              printf("[%s %d] failed err = %d\n","open_video_stream",0x2b5,*piVar4);
            }
            av_image_get_buffer_size
                      (0x17,*(undefined4 *)(*(int *)((int)param_1 + 0x14) + 0x5c),
                       *(undefined4 *)(*(int *)((int)param_1 + 0x14) + 0x60),1);
            iVar2 = av_buffer_alloc_frame();
            *(int *)((int)param_1 + 0xa68) = iVar2;
            if (iVar2 == 0) {
              puts("av_buffer_alloc_frame() failed");
LAB_0003fe18:
              puts("open_video_playing failed");
              return 0xffffffff;
            }
            iVar2 = av_image_fill_arrays
                              (*(int *)((int)param_1 + 0xa64),*(int *)((int)param_1 + 0xa64) + 0x20,
                               *(undefined4 *)(iVar2 + 4),0x17,
                               *(undefined4 *)(*(int *)((int)param_1 + 0x14) + 0x5c),
                               *(undefined4 *)(*(int *)((int)param_1 + 0x14) + 0x60),1);
            if (iVar2 < 0) {
              printf("av_image_fill_arrays() failed %d\n");
              goto LAB_0003fe18;
            }
            iVar2 = *(int *)((int)param_1 + 0x14);
            switch(*(undefined4 *)(iVar2 + 0x70)) {
            case 0xc:
              *(undefined4 *)(iVar2 + 0x70) = 0;
              break;
            case 0xd:
              *(undefined4 *)(iVar2 + 0x70) = 4;
              break;
            case 0xe:
              *(undefined4 *)(iVar2 + 0x70) = 5;
              break;
            case 0x20:
              *(undefined4 *)(iVar2 + 0x70) = 0x1f;
            }
            puVar3 = (undefined4 *)av_pix_fmt_desc_get();
            printf("convert [%s] res [%dx%d]\n",*puVar3,
                   *(undefined4 *)(*(int *)((int)param_1 + 0x14) + 0x5c),
                   *(undefined4 *)(*(int *)((int)param_1 + 0x14) + 0x60));
            iVar2 = pthread_create((pthread_t *)((int)param_1 + 0xb88),(pthread_attr_t *)0x0,
                                   (__start_routine *)&LAB_0003f7d0_1,param_1);
            if (iVar2 != 0) {
              piVar4 = __errno_location();
              printf("[%s %d] failed err = %d\n","open_video_playing",0x266,*piVar4);
            }
            goto LAB_0003fc94;
          }
          puts("avcodec_open2() failed");
        }
      }
    }
    puts("open_video_stream failed");
    uVar1 = 0xffffffff;
  }
  return uVar1;
}


