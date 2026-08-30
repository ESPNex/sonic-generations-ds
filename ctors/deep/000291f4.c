
void FUN_000291f4(int param_1)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  float fVar6;
  
  iVar4 = DAT_00029440;
  if ((*(char *)(*(int *)(DAT_00029440 + 4) + 0xfa15) == '\0') ||
     (iVar2 = FUN_00048964(DAT_00029444,param_1 + 0x1c), iVar2 == 0)) {
    sVar1 = *(short *)(param_1 + 0x184) + -1;
    *(short *)(param_1 + 0x184) = sVar1;
    if (sVar1 != 0) {
      if (sVar1 < 0xd8) {
        uVar3 = FUN_0022ec70();
        iVar2 = FUN_0020f338(param_1 + 0x168,uVar3);
        if (iVar2 != 0) {
          FUN_0021aa28(1,0);
          FUN_0021b340(0xdb,param_1 + 0x168);
          *(ushort *)(param_1 + 0x186) = *(ushort *)(param_1 + 0x186) | 1;
          *(undefined1 *)(*(int *)(iVar4 + 4) + 0xfa14) = 1;
          return;
        }
        iVar4 = FUN_0022a688();
        if (iVar4 != 0) {
          uVar3 = FUN_00230308();
          iVar4 = FUN_0020f338(param_1 + 0x168,uVar3);
          if (iVar4 != 0) {
            FUN_0021b340(0xdb,param_1 + 0x168);
            uVar3 = FUN_00230308();
            FUN_00225390(uVar3,0xe);
            goto LAB_00029244;
          }
        }
      }
      if ((short)*(ushort *)(param_1 + 0x184) < 0x20) {
        if (((*(ushort *)(param_1 + 0x186) & 2) == 0) && ((*(ushort *)(param_1 + 0x184) & 2) != 0))
        {
          FUN_00218a30(*(undefined4 *)(param_1 + 0x164));
        }
        else {
          FUN_00218a14(*(undefined4 *)(param_1 + 0x164));
        }
      }
      iVar4 = FUN_0020d5e4(param_1 + 4);
      fVar6 = DAT_00029448;
      if (iVar4 != 0) {
        *(uint *)(param_1 + 0x58) = *(uint *)(param_1 + 0x58) | 2;
      }
      if ((*(uint *)(param_1 + 0x98) & 1) == 0) {
        *(float *)(param_1 + 0x78) = *(float *)(param_1 + 0x78) + *(float *)(param_1 + 0x7c);
        uVar5 = *(uint *)(param_1 + 0x98);
      }
      else {
        *(float *)(param_1 + 0x78) = *(float *)(param_1 + 0x78) * fVar6;
        uVar5 = *(uint *)(param_1 + 0x98) & 0xfffffffe;
        *(uint *)(param_1 + 0x98) = uVar5;
      }
      if ((uVar5 & 2) != 0) {
        *(float *)(param_1 + 0x78) = *(float *)(param_1 + 0x78) * fVar6;
      }
      if ((*(uint *)(param_1 + 0x98) & 0xc) != 0) {
        *(float *)(param_1 + 0x68) = *(float *)(param_1 + 0x68) * fVar6;
      }
      FUN_00220630(param_1 + 4,0);
      *(float *)(param_1 + 0x1c) = *(float *)(param_1 + 0x1c) + *(float *)(param_1 + 0x34);
      *(float *)(param_1 + 0x20) = *(float *)(param_1 + 0x20) + *(float *)(param_1 + 0x38);
      fVar6 = *(float *)(param_1 + 0x24) + *(float *)(param_1 + 0x3c);
      *(float *)(param_1 + 0x24) = fVar6;
      *(undefined4 *)(param_1 + 0x168) = *(undefined4 *)(param_1 + 0x1c);
      *(float *)(param_1 + 0x16c) = *(float *)(param_1 + 0x20);
      *(float *)(param_1 + 0x170) = fVar6;
      *(float *)(param_1 + 0x16c) = *(float *)(param_1 + 0x20) + DAT_0002944c;
      FUN_0021b324(*(undefined4 *)(param_1 + 0x164),param_1 + 0x168);
      return;
    }
  }
LAB_00029244:
  *(ushort *)(param_1 + 0x186) = *(ushort *)(param_1 + 0x186) | 1;
  return;
}

