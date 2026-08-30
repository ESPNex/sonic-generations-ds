
void FUN_0003ad30(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  if (*(char *)(*(int *)(DAT_0003aec8 + 4) + 0xfa15) != '\0') {
    iVar1 = FUN_0020f4f8(DAT_0003aecc,param_1 + 0x168);
    if (iVar1 == 0) {
      FUN_00218a30(*(undefined4 *)(param_1 + 0x164));
    }
    else if (iVar1 == 1) {
      FUN_00218a14(*(undefined4 *)(param_1 + 0x164));
    }
    else {
      if (iVar1 == 2) {
        iVar1 = *(int *)(param_1 + 0x188);
        if (iVar1 != 0) {
          *(ushort *)(iVar1 + 2) = *(ushort *)(iVar1 + 2) & 0x7fff;
        }
        goto LAB_0003ae20;
      }
      if ((iVar1 == 3) && (iVar1 = FUN_0020f4e0(*(undefined4 *)(param_1 + 0x164)), iVar1 != 0)) {
        return;
      }
    }
  }
  uVar2 = FUN_0022ec70();
  iVar1 = FUN_0020f338(param_1 + 0x168,uVar2);
  if (iVar1 == 0) {
    iVar1 = FUN_0020f1dc(param_1);
    if (iVar1 == 0) {
      iVar1 = FUN_0022a688();
      if (iVar1 == 0) {
        return;
      }
      uVar2 = FUN_00230308();
      iVar1 = FUN_0020f338(param_1 + 0x168,uVar2);
      if (iVar1 == 0) {
        return;
      }
      FUN_0021b340(0xdb,param_1 + 0x168);
      uVar2 = FUN_00230308();
      FUN_00225390(uVar2,0xe);
    }
    else {
      FUN_0020f304(1);
      FUN_0020f144(*(undefined4 *)(param_1 + 0x168),*(undefined4 *)(param_1 + 0x16c),
                   *(undefined4 *)(param_1 + 0x170));
    }
  }
  else {
    FUN_0021aa28(1,1,0);
    FUN_0020f304(1);
    FUN_0021b340(0xdb,param_1 + 0x168);
  }
LAB_0003ae20:
  *(ushort *)(param_1 + 0x186) = *(ushort *)(param_1 + 0x186) | 1;
  return;
}

