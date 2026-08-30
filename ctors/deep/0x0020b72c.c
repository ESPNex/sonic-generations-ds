
void forced_0x0020b72c(undefined4 param_1,float param_2,float param_3,int param_4,int param_5,
                      int param_6)

{
  float *pfVar1;
  int iVar2;
  undefined2 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  int unaff_r4;
  int unaff_r5;
  bool bVar6;
  bool in_ZR;
  bool bVar7;
  bool bVar8;
  float fVar9;
  float fVar10;
  
  if (in_ZR) {
    if (param_6 != 0) {
      *(undefined4 *)(unaff_r4 + 0x16c) = param_1;
      uVar3 = 0;
LAB_0020b784:
      *(undefined2 *)(param_4 + 0xca) = uVar3;
      goto code_r0x0020b7c0;
    }
    fVar9 = *(float *)(unaff_r4 + 0x16c) + param_2;
    *(float *)(unaff_r4 + 0x16c) = fVar9;
    if ((int)fVar9 <= param_5) goto code_r0x0020b7c0;
    fVar9 = fVar9 - param_2;
    *(float *)(unaff_r4 + 0x16c) = fVar9;
  }
  else {
    if (param_6 == 0) {
      *(undefined4 *)(unaff_r4 + 0x16c) = param_1;
      uVar3 = 0x8000;
      goto LAB_0020b784;
    }
    fVar9 = *(float *)(unaff_r4 + 0x16c) + param_2;
    *(float *)(unaff_r4 + 0x16c) = fVar9;
    if ((int)fVar9 <= param_5) goto code_r0x0020b7c0;
    fVar9 = fVar9 - param_2;
    *(float *)(unaff_r4 + 0x16c) = fVar9;
  }
  if ((int)fVar9 <= param_5) {
    fVar9 = param_3;
  }
  *(float *)(unaff_r4 + 0x16c) = fVar9;
code_r0x0020b7c0:
  pfVar1 = (float *)FUN_003123e0();
  if (((*(uint *)(unaff_r4 + 0x19c) | *(uint *)(unaff_r4 + 0x1a0)) & 1) == 0) {
    fVar10 = *(float *)(unaff_r5 + 0x1f8);
    fVar9 = *pfVar1;
    bVar6 = fVar10 < fVar9;
    bVar8 = NAN(fVar10) || NAN(fVar9);
    if (fVar10 <= fVar9) {
      fVar10 = *(float *)(unaff_r5 + 0x200);
      bVar6 = fVar9 < fVar10;
      bVar8 = NAN(fVar9) || NAN(fVar10);
    }
    bVar7 = fVar10 == fVar9;
    if (bVar7 || bVar6 != bVar8) {
      fVar10 = *(float *)(unaff_r5 + 0x204);
      fVar9 = pfVar1[1];
      bVar6 = fVar10 < fVar9;
      bVar7 = fVar10 == fVar9;
      bVar8 = NAN(fVar10) || NAN(fVar9);
    }
    if ((!bVar7 && bVar6 == bVar8) || (*(float *)(unaff_r5 + 0x1fc) < fVar9)) {
      *(uint *)(unaff_r4 + 0xe0c) = *(uint *)(unaff_r5 + 0x208) | *(uint *)(unaff_r4 + 0xe0c);
      *(uint *)(unaff_r4 + 0xe00) = *(uint *)(unaff_r4 + 0xe14) & *(uint *)(unaff_r4 + 0xe10);
      uVar5 = *(undefined4 *)(DAT_0020b870 + 8);
      uVar4 = *(undefined4 *)(DAT_0020b870 + 0xc);
      *(undefined4 *)(*(int *)(unaff_r5 + 4) + 0x18) = DAT_0020b874;
      iVar2 = *(int *)(unaff_r5 + 4);
      *(undefined4 *)(iVar2 + 0x114) = uVar5;
      *(undefined4 *)(iVar2 + 0x118) = uVar4;
    }
  }
  return;
}

