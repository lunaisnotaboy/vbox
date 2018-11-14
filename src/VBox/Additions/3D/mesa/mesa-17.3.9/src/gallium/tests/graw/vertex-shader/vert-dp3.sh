VERT

DCL IN[0]
DCL IN[1]
DCL OUT[0], POSITION
DCL OUT[1], COLOR
DCL TEMP[0]

IMM FLT32 { 0.0, 0.0, 1.0, 1.0 }

DP3 TEMP[0].xy, IN[0], IN[0]
MOV TEMP[0].zw, IMM[0]
MUL OUT[0], IN[0], TEMP[0]
MOV OUT[1], IN[1]

END
