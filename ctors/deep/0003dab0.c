
void FUN_0003dab0(void)

{
  byte bVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  int iVar7;
  int iVar8;
  undefined4 uVar9;
  ushort *puVar10;
  int iVar11;
  ushort *puVar12;
  int iVar13;
  uint in_fpscr;
  uint uVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  float local_54;
  float local_50;
  float local_4c;
  
  switchD_0010db18::caseD_a();
  fVar6 = DAT_0003dc60;
  fVar5 = DAT_0003dc5c;
  fVar4 = DAT_0003dc58;
  fVar3 = DAT_0003dc54;
  fVar2 = DAT_0003dc50;
  iVar7 = **(int **)(DAT_0003dc4c + 4);
  if (iVar7 != -1) {
    iVar7 = iVar7 + (int)*(int **)(DAT_0003dc4c + 4);
    while( true ) {
      puVar12 = (ushort *)(iVar7 + 0x18);
      iVar13 = 0;
      fVar22 = (float)VectorUnsignedToFloat
                                ((uint)*(ushort *)(iVar7 + 0x10),(byte)(in_fpscr >> 0x16) & 3);
      if (*(short *)(iVar7 + 0x16) != 0) {
        do {
          puVar10 = puVar12 + 4;
          iVar11 = 0;
          if (*puVar12 != 0) {
            do {
              if ((puVar10[1] & 0x8000) == 0) {
                uVar14 = in_fpscr & 0xfffffff | (uint)(fVar22 == fVar5) << 0x1e;
                fVar15 = (float)VectorUnsignedToFloat
                                          ((uint)(byte)*puVar10,(byte)(uVar14 >> 0x16) & 3);
                fVar15 = fVar15 * fVar2;
                fVar16 = (float)VectorUnsignedToFloat
                                          ((uint)*(byte *)((int)puVar10 + 1),
                                           (byte)(uVar14 >> 0x16) & 3);
                fVar17 = (float)VectorSignedToFloat((int)(short)puVar12[1],
                                                    (byte)(uVar14 >> 0x16) & 3);
                fVar18 = (float)VectorSignedToFloat((int)((uint)puVar10[1] << 0x11) >> 0x11,
                                                    (byte)(uVar14 >> 0x16) & 3);
                fVar21 = -fVar18 - fVar4;
                fVar18 = fVar15;
                if (!SUB41(uVar14 >> 0x1e,0)) {
                  fVar19 = (float)FUN_00222f68(fVar22 * fVar6);
                  fVar20 = (float)FUN_002255d8(fVar22 * fVar6);
                  fVar18 = fVar15 * fVar19 + fVar21 * fVar20;
                  fVar21 = fVar21 * fVar19 - fVar15 * fVar20;
                }
                iVar8 = FUN_00230308();
                fVar15 = *(float *)(iVar8 + 0x80);
                fVar18 = *(float *)(iVar7 + 8) + fVar18;
                uVar14 = uVar14 & 0xfffffff | (uint)(fVar15 < fVar18) << 0x1f |
                         (uint)(fVar15 == fVar18) << 0x1e;
                in_fpscr = uVar14 | (uint)(NAN(fVar15) || NAN(fVar18)) << 0x1c;
                bVar1 = (byte)(uVar14 >> 0x18);
                if (!(bool)(bVar1 >> 6 & 1) && bVar1 >> 7 == ((byte)(in_fpscr >> 0x1c) & 1)) {
                  local_4c = *(float *)(iVar7 + 0xc) + fVar21;
                  local_54 = fVar18;
                  local_50 = fVar16 * fVar2 + fVar17 * fVar3;
                  uVar9 = FUN_00230308();
                  iVar8 = FUN_0020f338(&local_54,uVar9);
                  if (iVar8 != 0) {
                    puVar10[1] = puVar10[1] | 0x8000;
                  }
                }
              }
              iVar11 = iVar11 + 1;
              puVar10 = puVar10 + 3;
            } while (iVar11 < (int)(uint)*puVar12);
          }
          iVar13 = iVar13 + 1;
          puVar12 = (ushort *)((int)puVar12 + *(int *)(puVar12 + 2));
        } while (iVar13 < (int)(uint)*(ushort *)(iVar7 + 0x16));
      }
      if (*(int *)(iVar7 + 4) == 0) break;
      iVar7 = iVar7 + *(int *)(iVar7 + 4);
    }
  }
  return;
}

