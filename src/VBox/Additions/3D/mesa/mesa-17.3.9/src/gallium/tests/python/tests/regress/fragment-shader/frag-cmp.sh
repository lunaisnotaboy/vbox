FRAG

DCL IN[0], COLOR, LINEAR
DCL OUT[0], COLOR

IMM FLT32 { 1, 0, 0, 1 }
IMM FLT32 { 0, 1, 1, 0 }
IMM FLT32 { 1, 0,-1, 0 }

CMP OUT[0], IMM[2], IMM[0], IMM[1]

END
