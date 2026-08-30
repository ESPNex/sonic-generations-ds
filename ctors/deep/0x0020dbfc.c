
void forced_0x0020dbfc(void)

{
  undefined4 uVar1;
  int *piVar2;
  int unaff_r5;
  
  piVar2 = *(int **)(*(int *)(unaff_r5 + 0x10) + 0xc);
  (**(code **)(*piVar2 + 0x10))(&stack0x00000008,piVar2,*(undefined4 *)(DAT_0020dc94 + 0x38));
  FUN_0031fd78(*(undefined4 *)(unaff_r5 + 0x10),0,0,&stack0x00000008);
  uVar1 = FUN_002f8440(0,0,*(undefined4 *)(unaff_r5 + 0x10),*(undefined2 *)(unaff_r5 + 8));
  *(undefined4 *)(unaff_r5 + 0x18) = uVar1;
  uVar1 = FUN_002f8440(0,1,*(undefined4 *)(unaff_r5 + 0x10),*(undefined2 *)(unaff_r5 + 8));
  *(undefined4 *)(unaff_r5 + 0x1c) = uVar1;
  uVar1 = FUN_002f8440(0,2,*(undefined4 *)(unaff_r5 + 0x10),*(undefined2 *)(unaff_r5 + 8));
  *(undefined4 *)(unaff_r5 + 0x20) = uVar1;
  return;
}

