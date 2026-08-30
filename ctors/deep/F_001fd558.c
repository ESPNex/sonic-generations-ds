
void FUN_001fd558(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  byte bVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  
  iVar1 = DAT_001fd654;
  uVar5 = DAT_001fd650;
  iVar7 = DAT_001fd64c;
  if (*(int *)(param_2 + 4) != param_1) {
    iVar6 = 0;
    *(int *)(param_2 + 4) = param_1;
    do {
      iVar4 = *(int *)(iVar1 + iVar6 * 4);
      iVar2 = param_2 + iVar6 * 4;
      if (*(int *)(param_2 + 4) == iVar6) {
        **(undefined1 **)(iVar2 + 0x24) = 1;
        FUN_00328054(*(undefined4 *)(iVar2 + 0x24),1,*(undefined4 *)(iVar7 + iVar4 * 4));
      }
      else {
        *(undefined4 *)(*(int *)(iVar2 + 0x24) + 0xc) = uVar5;
        **(undefined1 **)(iVar2 + 0x24) = 0;
      }
      iVar2 = DAT_001fd658;
      iVar6 = iVar6 + 1;
    } while (iVar6 < 6);
    iVar7 = 0;
    do {
      uVar5 = *(undefined4 *)(iVar2 + *(int *)(iVar1 + iVar7 * 4) * 4);
      if (*(int *)(param_2 + 4) == iVar7) {
        iVar6 = FUN_0032c4bc(*(undefined4 *)(param_2 + 0x14),uVar5);
        bVar3 = *(byte *)(iVar6 + 0xb7) & 0xfe | 1;
      }
      else {
        iVar6 = FUN_0032c4bc(*(undefined4 *)(param_2 + 0x14),uVar5);
        bVar3 = *(byte *)(iVar6 + 0xb7) & 0xfe;
      }
      iVar7 = iVar7 + 1;
      *(byte *)(iVar6 + 0xb7) = bVar3;
    } while (iVar7 < 6);
    FUN_002d72a8(param_2);
    return;
  }
  return;
}

