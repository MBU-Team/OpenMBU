//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

/* lex -a -P console\yylex.c -p CMD -o console\scan.cc console\scan.l */
#define YYNEWLINE 10
#define INITIAL 0
#define CMD_endst 180
#define CMD_nxtmax 2057
#define YY_LA_SIZE 23

static unsigned int CMD_la_act[] = {
 0, 81, 41, 81, 2, 81, 3, 81, 81, 56, 81, 46, 81, 43, 81, 42,
 81, 52, 81, 44, 81, 47, 81, 39, 81, 38, 81, 81, 40, 81, 53, 81,
 54, 81, 76, 81, 76, 81, 76, 81, 32, 81, 33, 81, 34, 81, 35, 81,
 36, 81, 37, 81, 45, 81, 48, 81, 49, 81, 50, 81, 51, 81, 55, 81,
 76, 81, 76, 81, 76, 81, 76, 81, 76, 81, 76, 81, 76, 81, 76, 81,
 76, 81, 76, 81, 76, 81, 76, 81, 76, 81, 76, 81, 78, 81, 78, 81,
 81, 78, 79, 79, 77, 76, 76, 76, 73, 76, 76, 76, 76, 76, 76, 72,
 76, 76, 76, 76, 76, 70, 76, 69, 76, 76, 76, 76, 76, 76, 71, 76,
 76, 76, 76, 76, 76, 76, 67, 76, 76, 66, 76, 76, 76, 76, 68, 76,
 76, 76, 76, 76, 76, 64, 76, 76, 76, 76, 76, 76, 74, 76, 76, 76,
 76, 76, 76, 65, 76, 63, 76, 62, 76, 76, 76, 76, 61, 76, 76, 76,
 60, 76, 76, 76, 76, 76, 59, 76, 76, 76, 76, 58, 76, 57, 76, 76,
 31, 76, 76, 30, 76, 29, 76, 25, 23, 75, 80, 80, 75, 21, 15, 14,
 19, 13, 20, 12, 11, 26, 10, 24, 9, 17, 27, 8, 18, 28, 7, 16,
 6, 5, 4, 1, 22, 1, 0, 0
};

static unsigned char CMD_look[] = {
 0
};

static int CMD_final[] = {
 0, 0, 2, 4, 6, 7, 8, 9, 11, 13, 15, 17, 19, 21, 23, 25,
 27, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56,
 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88,
 90, 92, 94, 96, 97, 98, 98, 98, 98, 99, 100, 100, 101, 102, 103, 104,
 106, 107, 108, 109, 110, 111, 113, 114, 115, 116, 117, 119, 120, 121, 122, 123,
 124, 125, 126, 128, 129, 130, 131, 132, 133, 134, 136, 137, 139, 140, 141, 142,
 144, 145, 146, 147, 148, 149, 151, 152, 153, 154, 155, 156, 158, 159, 160, 161,
 162, 163, 165, 167, 169, 170, 171, 172, 174, 175, 176, 178, 179, 180, 181, 182,
 184, 185, 186, 187, 189, 191, 192, 194, 195, 197, 199, 200, 201, 202, 202, 203,
 204, 204, 205, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
 218, 219, 220, 221, 222, 223, 223, 224, 225, 225, 225, 226, 226, 226, 226, 227,
 227, 228, 229, 230, 231
};
#ifndef CMD_state_t
#define CMD_state_t unsigned char
#endif

static CMD_state_t CMD_begin[] = {
 0, 0, 0
};

static CMD_state_t CMD_next[] = {
 51, 51, 51, 51, 51, 51, 51, 51, 51, 1, 4, 1, 1, 3, 51, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 1, 8, 5, 51, 16, 18, 11, 6, 27, 28, 17, 15, 33, 14, 29, 2,
 49, 50, 50, 50, 50, 50, 50, 50, 50, 50, 13, 30, 10, 7, 9, 24,
 23, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 20, 48,
 48, 48, 48, 22, 21, 48, 48, 48, 48, 48, 48, 25, 51, 26, 19, 48,
 51, 48, 36, 42, 44, 38, 41, 48, 48, 40, 48, 48, 48, 48, 43, 35,
 46, 48, 37, 45, 47, 48, 48, 39, 48, 48, 48, 31, 12, 32, 34, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
 53, 61, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 180, 180, 180, 180,
 54, 58, 62, 55, 63, 55, 64, 54, 56, 56, 56, 56, 56, 56, 56, 56,
 56, 56, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 65, 66, 67, 68,
 54, 58, 69, 70, 71, 72, 73, 54, 59, 59, 59, 59, 59, 59, 59, 59,
 59, 59, 74, 75, 76, 78, 79, 80, 77, 59, 59, 59, 59, 59, 59, 81,
 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 93, 94, 95, 96, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 92, 59, 59, 59, 59, 59, 59, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 97, 98, 99, 100, 60, 101, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 104, 105, 106, 107, 108, 109, 110,
 111, 112, 113, 114, 115, 116, 117, 102, 118, 119, 120, 121, 122, 103, 123, 124,
 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 141, 141,
 141, 141, 141, 141, 141, 141, 141, 141, 147, 148, 153, 139, 155, 159, 158, 140,
 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
 140, 140, 140, 140, 140, 140, 140, 140, 140, 160, 161, 162, 163, 140, 166, 140,
 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
 140, 140, 140, 140, 140, 140, 140, 140, 140, 180, 167, 154, 180, 142, 142, 142,
 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
 142, 142, 142, 142, 142, 142, 142, 180, 180, 180, 180, 142, 180, 142, 142, 142,
 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
 142, 142, 142, 142, 142, 142, 142, 143, 143, 143, 143, 143, 143, 143, 143, 143,
 143, 144, 180, 180, 180, 180, 180, 180, 143, 143, 143, 143, 143, 143, 143, 143,
 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143,
 143, 143, 180, 180, 180, 180, 143, 180, 143, 143, 143, 143, 143, 143, 143, 143,
 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143,
 143, 143, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 146, 180, 180, 180,
 180, 180, 180, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 180, 180, 180,
 180, 145, 180, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 149, 151, 156,
 165, 179, 176, 179, 179, 180, 180, 180, 180, 180, 180, 180, 180, 180, 152, 150,
 177, 180, 180, 180, 180, 180, 157, 180, 179, 164, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 180, 169, 169, 180, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 170, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 168, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
 169, 169, 169, 169, 169, 169, 169, 169, 169, 169, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 180, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171,
 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 180, 173, 173, 180, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 174, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 172, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 173,
 173, 173, 173, 173, 173, 173, 173, 173, 173, 173, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 180, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 175,
 175, 175, 175, 175, 175, 175, 175, 175, 175, 175, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 180, 178, 178, 180, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 178,
 178, 178, 178, 178, 178, 178, 178, 178, 178, 178, 0
};

static CMD_state_t CMD_check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 50, 47, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 56, 55, 56, 55,
 57, 49, 61, 54, 62, 54, 46, 50, 54, 54, 54, 54, 54, 54, 54, 54,
 54, 54, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 64, 65, 66, 67,
 57, 49, 68, 45, 70, 71, 72, 50, 58, 58, 58, 58, 58, 58, 58, 58,
 58, 58, 73, 74, 44, 77, 78, 79, 44, 58, 58, 58, 58, 58, 58, 80,
 81, 76, 83, 84, 85, 86, 87, 88, 43, 90, 42, 93, 94, 92, 48, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 42, 58, 58, 58, 58, 58, 58, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 96, 97, 98, 99, 48, 100, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 41, 104, 105, 106, 103, 108, 109,
 110, 111, 112, 102, 40, 39, 116, 41, 117, 118, 38, 120, 121, 41, 37, 123,
 124, 125, 126, 36, 128, 129, 130, 35, 22, 133, 21, 135, 20, 19, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 17, 16, 13, 18, 12, 10, 10, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 159, 9, 9, 162, 18, 165, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 141, 7, 12, ~0U, 141, 141, 141,
 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141,
 141, 141, 141, 141, 141, 141, 141, ~0U, ~0U, ~0U, ~0U, 141, ~0U, 141, 141, 141,
 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141,
 141, 141, 141, 141, 141, 141, 141, 142, 142, 142, 142, 142, 142, 142, 142, 142,
 142, 142, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, 142, 142, 142, 142, 142, 142, 142, 142,
 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
 142, 142, ~0U, ~0U, ~0U, ~0U, 142, ~0U, 142, 142, 142, 142, 142, 142, 142, 142,
 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
 142, 142, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, ~0U, ~0U, ~0U,
 ~0U, ~0U, ~0U, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, ~0U, ~0U, ~0U,
 ~0U, 140, ~0U, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 15, 14, 11,
 8, 1, 2, 1, 1, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, 14, 15,
 2, ~0U, ~0U, ~0U, ~0U, ~0U, 11, ~0U, 1, 8, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, ~0U, 6, 6, ~0U, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, ~0U, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168,
 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, ~0U, 5, 5, ~0U, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, ~0U, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 172,
 172, 172, 172, 172, 172, 172, 172, 172, 172, 172, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, ~0U, 176, 176, ~0U, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176,
 176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 0
};

static CMD_state_t CMD_default[] = {
 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180,
 18, 180, 180, 180, 48, 48, 48, 180, 180, 180, 180, 180, 180, 180, 180, 180,
 180, 180, 180, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
 180, 50, 180, 180, 50, 180, 180, 54, 54, 53, 180, 58, 48, 48, 48, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 180, 48, 48, 48, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 180, 180, 180, 18, 180, 142,
 142, 140, 140, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180,
 180, 180, 180, 180, 180, 180, 180, 180, 180, 6, 180, 6, 180, 5, 180, 5,
 180, 180, 176, 1, 0
};

static int CMD_base[] = {
 0, 744, 707, 2058, 2058, 1290, 778, 477, 716, 445, 417, 713, 415, 416, 705, 706,
 412, 411, 414, 400, 384, 393, 376, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058,
 2058, 2058, 2058, 341, 337, 345, 334, 333, 334, 328, 249, 243, 227, 188, 181, 143,
 302, 185, 210, 2058, 2058, 242, 232, 226, 225, 203, 264, 2058, 2058, 157, 175, 2058,
 201, 194, 205, 200, 205, 2058, 203, 193, 211, 218, 287, 2058, 221, 223, 229, 210,
 227, 220, 2058, 241, 241, 232, 230, 243, 236, 2058, 226, 2058, 239, 232, 247, 2058,
 277, 289, 285, 279, 297, 2058, 321, 319, 318, 312, 327, 2058, 331, 315, 327, 322,
 324, 2058, 2058, 2058, 333, 332, 340, 2058, 328, 343, 2058, 331, 331, 335, 340, 2058,
 351, 356, 347, 2058, 2058, 390, 2058, 393, 2058, 2058, 2058, 2058, 626, 476, 551, 2058,
 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 2058, 444,
 2058, 2058, 447, 2058, 2058, 449, 2058, 2058, 1034, 2058, 2058, 2058, 1546, 2058, 2058, 2058,
 1802, 2058, 2058, 2058, 2058
};


#line 1 "console/yylex.c"
/*
 * Copyright 1988, 1992 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 *
 * $Header: /cvs/tse/tse/engine/console/scan.cpp,v 1.6 2006/09/13 23:32:26 bramage Exp $
 *
 */
#include <stdlib.h>
#include <stdio.h>
#if	__STDC__
#define YY_ARGS(args)	args
#else
#define YY_ARGS(args)	()
#endif

#ifdef LEX_WINDOWS
#include <windows.h>

/*
 * define, if not already defined
 * the flag YYEXIT, which will allow
 * graceful exits from CMDlex()
 * without resorting to calling exit();
 */

#ifndef YYEXIT
#define YYEXIT	1
#endif

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling CMDlex must supply this!
 */

#ifdef STRICT
extern HINSTANCE hInst;	
#else
extern HANDLE hInst;	
#endif

#endif	/* LEX_WINDOWS */

/*
 * Define m_textmsg() to an appropriate function for internationalized messages
 * or custom processing.
 */
#ifndef I18N
#define	m_textmsg(id, str, cls)	(str)
#else /*I18N*/
extern	char* m_textmsg YY_ARGS((int id, const char* str, char* cls));
#endif/*I18N*/

/*
 * Include string.h to get definition of memmove() and size_t.
 * If you do not have string.h or it does not declare memmove
 * or size_t, you will have to declare them here.
 */
#include <string.h>
/* Uncomment next line if memmove() is not declared in string.h */
/*extern char * memmove();*/
/* Uncomment next line if size_t is not available in stdio.h or string.h */
/*typedef unsigned size_t;*/
/* Drop this when LATTICE provides memmove */
#ifdef LATTICE
#define memmove	memcopy
#endif

/*
 * YY_STATIC determines the scope of variables and functions
 * declared by the lex scanner. It must be set with a -DYY_STATIC
 * option to the compiler (it cannot be defined in the lex program).
 */
#ifdef	YY_STATIC
/* define all variables as static to allow more than one lex scanner */
#define	YY_DECL	static
#else
/* define all variables as global to allow other modules to access them */
#define	YY_DECL	
#endif

/*
 * You can redefine CMDgetc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define CMDgetc()	yt_getc()
#else
#define	CMDgetc()	getc(CMDin) 	/* CMDlex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#ifdef YYEXIT
#define	YY_FATAL(msg)	{ fprintf(CMDout, "CMDlex: %s\n", msg); CMDLexFatal = 1; }
#else /* YYEXIT */
#define	YY_FATAL(msg)	{ fprintf(stderr, "CMDlex: %s\n", msg); exit(1); }
#endif /* YYEXIT */

#undef ECHO
#define	ECHO		fputs(CMDtext, CMDout)

#define	output(c)	putc((c), CMDout) /* CMDlex sink for unmatched chars */
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */

#define	BEGIN		CMD_start =
#define	REJECT		goto CMD_reject
#define	NLSTATE		(CMD_lastc = YYNEWLINE)
#define	YY_INIT \
	(CMD_start = CMDleng = CMD_end = 0, CMD_lastc = YYNEWLINE)
#define	CMDmore()	goto CMD_more
#define	CMDless(n)	if ((n) < 0 || (n) > CMD_end) ; \
			else { YY_SCANNER; CMDleng = (n); YY_USER; }

YY_DECL	void	CMD_reset YY_ARGS((void));
YY_DECL	int	input	YY_ARGS((void));
YY_DECL	int	unput	YY_ARGS((int c));

/* functions defined in libl.lib */
extern	int	CMDwrap	YY_ARGS((void));
extern	void	CMDerror	YY_ARGS((char *fmt, ...));
extern	void	CMDcomment	YY_ARGS((char *term));
extern	int	CMDmapch	YY_ARGS((int delim, int escape));

#line 1 "console/scan.l"

#define YYLMAX 4096

#include "platform/platform.h"
#include "console/console.h"
#include "console/ast.h"
#include "console/gram.h"
#include "core/stringTable.h"

static int Sc_ScanString(int ret);
static int Sc_ScanNum();
static int Sc_ScanVar();
static int Sc_ScanHex();

#define FLEX_DEBUG 1

//#undef input
//#undef unput
#undef CMDgetc
int CMDgetc();
static int lineIndex;
extern DataChunker consoleAllocator;

// Prototypes
void SetScanBuffer(const char *sb, const char *fn);
const char * CMDgetFileLine(int &lineNumber);
void CMDerror(char * s, ...);

#line 127 "console/yylex.c"


#ifndef YYLMAX
#define	YYLMAX		100		/* token and pushback buffer size */
#endif /* YYLMAX */

/*
 * If %array is used (or defaulted), CMDtext[] contains the token.
 * If %pointer is used, CMDtext is a pointer to CMD_tbuf[].
 */
YY_DECL char	CMDtext[YYLMAX+1];



#ifdef	YY_DEBUG
#undef	YY_DEBUG
#define	YY_DEBUG(fmt, a1, a2)	fprintf(stderr, fmt, a1, a2)
#else
#define	YY_DEBUG(fmt, a1, a2)
#endif

/*
 * The declaration for the lex scanner can be changed by
 * redefining YYLEX or YYDECL. This must be done if you have
 * more than one scanner in a program.
 */
#ifndef	YYLEX
#define	YYLEX CMDlex			/* name of lex scanner */
#endif

#ifndef YYDECL
#define	YYDECL	int YYLEX YY_ARGS((void))	/* declaration for lex scanner */
#endif

/*
 * stdin and stdout may not neccessarily be constants.
 * If stdin and stdout are constant, and you want to save a few cycles, then
 * #define YY_STATIC_STDIO 1 in this file or on the commandline when
 * compiling this file
 */
#ifndef YY_STATIC_STDIO
#define YY_STATIC_STDIO	0
#endif

#if YY_STATIC_STDIO
YY_DECL	FILE   *CMDin = stdin;
YY_DECL	FILE   *CMDout = stdout;
#else
YY_DECL	FILE   *CMDin = (FILE *)0;
YY_DECL	FILE   *CMDout = (FILE *)0;
#endif
YY_DECL	int	CMDlineno = 1;		/* line number */

/* CMD_sbuf[0:CMDleng-1] contains the states corresponding to CMDtext.
 * CMDtext[0:CMDleng-1] contains the current token.
 * CMDtext[CMDleng:CMD_end-1] contains pushed-back characters.
 * When the user action routine is active,
 * CMD_save contains CMDtext[CMDleng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */
static	CMD_state_t CMD_sbuf [YYLMAX+1];	/* state buffer */
static	int	CMD_end = 0;		/* end of pushback */
static	int	CMD_start = 0;		/* start state */
static	int	CMD_lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	CMDleng = 0;		/* CMDtext token length */
#ifdef YYEXIT
static	int CMDLexFatal;
#endif /* YYEXIT */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char CMD_save;	/* saved CMDtext[CMDleng] */

#define	YY_USER	{ /* set up CMDtext for user */ \
		CMD_save = CMDtext[CMDleng]; \
		CMDtext[CMDleng] = 0; \
	}
#define	YY_SCANNER { /* set up CMDtext for scanner */ \
		CMDtext[CMDleng] = CMD_save; \
	}

#else		/* not-so efficient push-back for CMDtext mungers */

static	char CMD_save [YYLMAX];
static	char *CMD_push = CMD_save+YYLMAX;

#define	YY_USER { \
		size_t n = CMD_end - CMDleng; \
		CMD_push = CMD_save+YYLMAX - n; \
		if (n > 0) \
			memmove(CMD_push, CMDtext+CMDleng, n); \
		CMDtext[CMDleng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = CMD_save+YYLMAX - CMD_push; \
		if (n > 0) \
			memmove(CMDtext+CMDleng, CMD_push, n); \
		CMD_end = CMDleng + n; \
	}

#endif


#ifdef LEX_WINDOWS

/*
 * When using the windows features of lex,
 * it is necessary to load in the resources being
 * used, and when done with them, the resources must
 * be freed up, otherwise we have a windows app that
 * is not following the rules. Thus, to make CMDlex()
 * behave in a windows environment, create a new
 * CMDlex() which will call the original CMDlex() as
 * another function call. Observe ...
 */

/*
 * The actual lex scanner (usually CMDlex(void)).
 * NOTE: you should invoke CMD_init() if you are calling CMDlex()
 * with new input; otherwise old lookaside will get in your way
 * and CMDlex() will die horribly.
 */
static int win_CMDlex();			/* prototype for windows CMDlex handler */

YYDECL {
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short far *old_CMD_la_act;	/* remember previous pointer values */
	short far *old_CMD_final;
	CMD_state_t far *old_CMD_begin;
	CMD_state_t far *old_CMD_next;
	CMD_state_t far *old_CMD_check;
	CMD_state_t far *old_CMD_default;
	short far *old_CMD_base;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_CMDLEX", "CMDLEXTBL"));
	
	/*
	 * return an error code if any
	 * of the resources did not load
	 */

	if (hRes_table == NULL)
		return (0);
	
	/*
	 * the following code will lock the resources
	 * into fixed memory locations for the scanner
	 * (and remember previous pointer locations)
	 */

	old_CMD_la_act = CMD_la_act;
	old_CMD_final = CMD_final;
	old_CMD_begin = CMD_begin;
	old_CMD_next = CMD_next;
	old_CMD_check = CMD_check;
	old_CMD_default = CMD_default;
	old_CMD_base = CMD_base;

	CMD_la_act = (unsigned short far *)LockResource (hRes_table);
	CMD_final = (short far *)(CMD_la_act + Sizeof_CMD_la_act);
	CMD_begin = (CMD_state_t far *)(CMD_final + Sizeof_CMD_final);
	CMD_next = (CMD_state_t far *)(CMD_begin + Sizeof_CMD_begin);
	CMD_check = (CMD_state_t far *)(CMD_next + Sizeof_CMD_next);
	CMD_default = (CMD_state_t far *)(CMD_check + Sizeof_CMD_check);
	CMD_base = (CMD_state_t far *)(CMD_default + Sizeof_CMD_default);


	/*
	 * call the standard CMDlex() code
	 */

	wReturnValue = win_CMDlex();

	/*
	 * unlock the resources
	 */

	UnlockResource (hRes_table);

	/*
	 * and now free the resource
	 */

	FreeResource (hRes_table);

	/*
	 * restore previously saved pointers
	 */

	CMD_la_act = old_CMD_la_act;
	CMD_final = old_CMD_final;
	CMD_begin = old_CMD_begin;
	CMD_next = old_CMD_next;
	CMD_check = old_CMD_check;
	CMD_default = old_CMD_default;
	CMD_base = old_CMD_base;

	return (wReturnValue);
}	/* end function */

static int win_CMDlex() {

#else /* LEX_WINDOWS */

/*
 * The actual lex scanner (usually CMDlex(void)).
 * NOTE: you should invoke CMD_init() if you are calling CMDlex()
 * with new input; otherwise old lookaside will get in your way
 * and CMDlex() will die horribly.
 */
YYDECL {

#endif /* LEX_WINDOWS */

	register int c, i, CMDbase;
	unsigned	CMDst;	/* state */
	int CMDfmin, CMDfmax;	/* CMD_la_act indices of final states */
	int CMDoldi, CMDoleng;	/* base i, CMDleng before look-ahead */
	int CMDeof;		/* 1 if eof has already been read */
#line 47 "console/scan.l"
	;

#line 350 "console/yylex.c"



#if !YY_STATIC_STDIO
	if (CMDin == (FILE *)0)
		CMDin = stdin;
	if (CMDout == (FILE *)0)
		CMDout = stdout;
#endif

#ifdef YYEXIT
	CMDLexFatal = 0;
#endif /* YYEXIT */

	CMDeof = 0;
	i = CMDleng;
	YY_SCANNER;

  CMD_again:
	CMDleng = i;
	/* determine previous char. */
	if (i > 0)
		CMD_lastc = CMDtext[i-1];
	/* scan previously accepted token adjusting CMDlineno */
	while (i > 0)
		if (CMDtext[--i] == YYNEWLINE)
			CMDlineno++;
	/* adjust pushback */
	CMD_end -= CMDleng;
	if (CMD_end > 0)
		memmove(CMDtext, CMDtext+CMDleng, (size_t) CMD_end);
	i = 0;

  CMD_contin:
	CMDoldi = i;

	/* run the state machine until it jams */
	CMDst = CMD_begin[CMD_start + ((CMD_lastc == YYNEWLINE) ? 1 : 0)];
	CMD_sbuf[i] = (CMD_state_t) CMDst;
	do {
		YY_DEBUG(m_textmsg(1547, "<state %d, i = %d>\n", "I num1 num2"), CMDst, i);
		if (i >= YYLMAX) {
			YY_FATAL(m_textmsg(1548, "Token buffer overflow", "E"));
#ifdef YYEXIT
			if (CMDLexFatal)
				return -2;
#endif /* YYEXIT */
		}	/* endif */

		/* get input char */
		if (i < CMD_end)
			c = CMDtext[i];		/* get pushback char */
		else if (!CMDeof && (c = CMDgetc()) != EOF) {
			CMD_end = i+1;
			CMDtext[i] = (char) c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == CMDoldi) {	/* no token */
				CMDeof = 0;
				if (CMDwrap())
					return 0;
				else
					goto CMD_again;
			} else {
				CMDeof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(m_textmsg(1549, "<input %d = 0x%02x>\n", "I num hexnum"), c, c);

		/* look up next state */
		while ((CMDbase = CMD_base[CMDst]+(unsigned char)c) > CMD_nxtmax
		    || CMD_check[CMDbase] != (CMD_state_t) CMDst) {
			if (CMDst == CMD_endst)
				goto CMD_jammed;
			CMDst = CMD_default[CMDst];
		}
		CMDst = CMD_next[CMDbase];
	  CMD_jammed: ;
	  CMD_sbuf[++i] = (CMD_state_t) CMDst;
	} while (!(CMDst == CMD_endst || YY_INTERACTIVE && CMD_base[CMDst] > CMD_nxtmax && CMD_default[CMDst] == CMD_endst));
	YY_DEBUG(m_textmsg(1550, "<stopped %d, i = %d>\n", "I num1 num2"), CMDst, i);
	if (CMDst != CMD_endst)
		++i;

  CMD_search:
	/* search backward for a final state */
	while (--i > CMDoldi) {
		CMDst = CMD_sbuf[i];
		if ((CMDfmin = CMD_final[CMDst]) < (CMDfmax = CMD_final[CMDst+1]))
			goto CMD_found;	/* found final state(s) */
	}
	/* no match, default action */
	i = CMDoldi + 1;
	output(CMDtext[CMDoldi]);
	goto CMD_again;

  CMD_found:
	YY_DEBUG(m_textmsg(1551, "<final state %d, i = %d>\n", "I num1 num2"), CMDst, i);
	CMDoleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(CMD_la_act[CMDfmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = CMD_look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < CMDoldi) {	/* no / */
				i = CMDoleng;
				break;
			}
			CMDst = CMD_sbuf[i];
			if (bv[(unsigned)CMDst/8] & bits[(unsigned)CMDst%8])
				break;
		}
	}

	/* perform action */
	CMDleng = i;
	YY_USER;
	switch (CMD_la_act[CMDfmin] & 0777) {
	case 0:
#line 48 "console/scan.l"
	{ }
	break;
	case 1:
#line 49 "console/scan.l"
	;
	break;
	case 2:
#line 50 "console/scan.l"
	;
	break;
	case 3:
#line 51 "console/scan.l"
	{lineIndex++;}
	break;
	case 4:
#line 52 "console/scan.l"
	{ return(Sc_ScanString(STRATOM)); }
	break;
	case 5:
#line 53 "console/scan.l"
	{ return(Sc_ScanString(TAGATOM)); }
	break;
	case 6:
#line 54 "console/scan.l"
	return(CMDlval.i = opEQ);
	break;
	case 7:
#line 55 "console/scan.l"
	return(CMDlval.i = opNE);
	break;
	case 8:
#line 56 "console/scan.l"
	return(CMDlval.i = opGE);
	break;
	case 9:
#line 57 "console/scan.l"
	return(CMDlval.i = opLE);
	break;
	case 10:
#line 58 "console/scan.l"
	return(CMDlval.i = opAND);
	break;
	case 11:
#line 59 "console/scan.l"
	return(CMDlval.i = opOR);
	break;
	case 12:
#line 60 "console/scan.l"
	return(CMDlval.i = opCOLONCOLON);
	break;
	case 13:
#line 61 "console/scan.l"
	return(CMDlval.i = opMINUSMINUS);
	break;
	case 14:
#line 62 "console/scan.l"
	return(CMDlval.i = opPLUSPLUS);
	break;
	case 15:
#line 63 "console/scan.l"
	return(CMDlval.i = opSTREQ);
	break;
	case 16:
#line 64 "console/scan.l"
	return(CMDlval.i = opSTRNE);
	break;
	case 17:
#line 65 "console/scan.l"
	return(CMDlval.i = opSHL);
	break;
	case 18:
#line 66 "console/scan.l"
	return(CMDlval.i = opSHR);
	break;
	case 19:
#line 67 "console/scan.l"
	return(CMDlval.i = opPLASN);
	break;
	case 20:
#line 68 "console/scan.l"
	return(CMDlval.i = opMIASN);
	break;
	case 21:
#line 69 "console/scan.l"
	return(CMDlval.i = opMLASN);
	break;
	case 22:
#line 70 "console/scan.l"
	return(CMDlval.i = opDVASN);
	break;
	case 23:
#line 71 "console/scan.l"
	return(CMDlval.i = opMODASN);
	break;
	case 24:
#line 72 "console/scan.l"
	return(CMDlval.i = opANDASN);
	break;
	case 25:
#line 73 "console/scan.l"
	return(CMDlval.i = opXORASN);
	break;
	case 26:
#line 74 "console/scan.l"
	return(CMDlval.i = opORASN);
	break;
	case 27:
#line 75 "console/scan.l"
	return(CMDlval.i = opSLASN);
	break;
	case 28:
#line 76 "console/scan.l"
	return(CMDlval.i = opSRASN);
	break;
	case 29:
#line 77 "console/scan.l"
	{CMDlval.i = '\n'; return '@'; }
	break;
	case 30:
#line 78 "console/scan.l"
	{CMDlval.i = '\t'; return '@'; }
	break;
	case 31:
#line 79 "console/scan.l"
	{CMDlval.i = ' '; return '@'; }
	break;
	case 32:
#line 80 "console/scan.l"
	{CMDlval.i = 0; return '@'; }
	break;
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51:
	case 52:
	case 53:
	case 54:
	case 55:
	case 56:
#line 104 "console/scan.l"
	{       return(CMDlval.i = CMDtext[0]); }
	break;
	case 57:
#line 105 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwCASEOR); }
	break;
	case 58:
#line 106 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwBREAK); }
	break;
	case 59:
#line 107 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwRETURN); }
	break;
	case 60:
#line 108 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwELSE); }
	break;
	case 61:
#line 109 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwWHILE); }
	break;
	case 62:
#line 110 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwIF); }
	break;
	case 63:
#line 111 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwFOR); }
	break;
	case 64:
#line 112 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwCONTINUE); }
	break;
	case 65:
#line 113 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwDEFINE); }
	break;
	case 66:
#line 114 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwDECLARE); }
	break;
	case 67:
#line 115 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwDATABLOCK); }
	break;
	case 68:
#line 116 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwCASE); }
	break;
	case 69:
#line 117 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwSWITCHSTR); }
	break;
	case 70:
#line 118 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwSWITCH); }
	break;
	case 71:
#line 119 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwDEFAULT); }
	break;
	case 72:
#line 120 "console/scan.l"
	{ CMDlval.i = lineIndex; return(rwPACKAGE); }
	break;
	case 73:
#line 121 "console/scan.l"
	{ CMDlval.i = 1; return INTCONST; }
	break;
	case 74:
#line 122 "console/scan.l"
	{ CMDlval.i = 0; return INTCONST; }
	break;
	case 75:
#line 123 "console/scan.l"
	return(Sc_ScanVar());
	break;
	case 76:
#line 124 "console/scan.l"
	{ CMDtext[CMDleng] = 0; CMDlval.s = StringTable->insert(CMDtext); return(IDENT); }
	break;
	case 77:
#line 125 "console/scan.l"
	return(Sc_ScanHex());
	break;
	case 78:
#line 126 "console/scan.l"
	{ CMDtext[CMDleng] = 0; CMDlval.i = atoi(CMDtext); return INTCONST; }
	break;
	case 79:
#line 127 "console/scan.l"
	return Sc_ScanNum();
	break;
	case 80:
#line 128 "console/scan.l"
	return(ILLEGAL_TOKEN);
	break;
	case 81:
#line 129 "console/scan.l"
	return(ILLEGAL_TOKEN);
	break;

#line 472 "console/yylex.c"

	}
	YY_SCANNER;
	i = CMDleng;
	goto CMD_again;			/* action fell though */

  CMD_reject:
	YY_SCANNER;
	i = CMDoleng;			/* restore original CMDtext */
	if (++CMDfmin < CMDfmax)
		goto CMD_found;		/* another final state, same length */
	else
		goto CMD_search;		/* try shorter CMDtext */

  CMD_more:
	YY_SCANNER;
	i = CMDleng;
	if (i > 0)
		CMD_lastc = CMDtext[i-1];
	goto CMD_contin;
}
/*
 * Safely switch input stream underneath LEX
 */
typedef struct CMD_save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	CMD_state_t	savestate[YYLMAX+1];
} YY_SAVED;

void
CMD_reset()
{
	YY_INIT;
	CMDlineno = 1;		/* line number */
}

#if 0
YY_SAVED *
CMDSaveScan(fp)
FILE * fp;
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = CMDin;
	p->oldline = CMDlineno;
	p->oldend = CMD_end;
	p->oldstart = CMD_start;
	p->oldlastc = CMD_lastc;
	p->oldleng = CMDleng;
	(void) memcpy(p->savetext, CMDtext, sizeof CMDtext);
	(void) memcpy((char *) p->savestate, (char *) CMD_sbuf,
		sizeof CMD_sbuf);

	CMDin = fp;
	CMDlineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
CMDRestoreScan(p)
YY_SAVED * p;
{
	if (p == NULL)
		return;
	CMDin = p->oldfp;
	CMDlineno = p->oldline;
	CMD_end = p->oldend;
	CMD_start = p->oldstart;
	CMD_lastc = p->oldlastc;
	CMDleng = p->oldleng;

	(void) memcpy(CMDtext, p->savetext, sizeof CMDtext);
	(void) memcpy((char *) CMD_sbuf, (char *) p->savestate,
		sizeof CMD_sbuf);
	free(p);
}
/*
 * User-callable re-initialization of CMDlex()
 */
/* get input char with pushback */
YY_DECL int
input()
{
	int c;
#ifndef YY_PRESERVE
	if (CMD_end > CMDleng) {
		CMD_end--;
		memmove(CMDtext+CMDleng, CMDtext+CMDleng+1,
			(size_t) (CMD_end-CMDleng));
		c = CMD_save;
		YY_USER;
#else
	if (CMD_push < CMD_save+YYLMAX) {
		c = *CMD_push++;
#endif
	} else
		c = CMDgetc();
	CMD_lastc = c;
	if (c == YYNEWLINE)
		CMDlineno++;
	if (c == EOF) /* CMDgetc() can set c=EOF vsc4 wants c==EOF to return 0 */
		return 0;
	else
		return c;
}

/*f
 * pushback char
 */
YY_DECL int
unput(c)
	int c;
{
#ifndef YY_PRESERVE
	if (CMD_end >= YYLMAX) {
		YY_FATAL(m_textmsg(1552, "Push-back buffer overflow", "E"));
	} else {
		if (CMD_end > CMDleng) {
			CMDtext[CMDleng] = CMD_save;
			memmove(CMDtext+CMDleng+1, CMDtext+CMDleng,
				(size_t) (CMD_end-CMDleng));
			CMDtext[CMDleng] = 0;
		}
		CMD_end++;
		CMD_save = (char) c;
#else
	if (CMD_push <= CMD_save) {
		YY_FATAL(m_textmsg(1552, "Push-back buffer overflow", "E"));
	} else {
		*--CMD_push = c;
#endif
		if (c == YYNEWLINE)
			CMDlineno--;
	}	/* endif */
	return c;
}

#endif

#line 131 "console/scan.l"

/*
 * Scan character constant.
 */

/*
 * Scan identifier.
 */

static const char *scanBuffer;
static const char *fileName;
static int scanIndex;
static int fakeLineIndex;
 
const char * CMDGetCurrentFile()
{
   return fileName;
}

int CMDGetCurrentLine()
{
   return lineIndex;
}

extern bool gConsoleSyntaxError;

void CMDerror(char *, ...)
{
   gConsoleSyntaxError = true;
   if(fileName)
   {
      const int BUFMAX = 256;
      char tempBuf[BUFMAX];
      Con::errorf(ConsoleLogEntry::Script, "%s Line: %d - Syntax error.",
         fileName, lineIndex);

#ifndef NO_ADVANCED_ERROR_REPORT
      // dhc - lineIndex is bogus.  let's try to add some sanity back in.
      int i,j,n;
      char c;
      int linediv = 1;
      // first, walk the buffer, trying to detect line ending type.
      // this is imperfect, if inconsistant line endings exist...
      for (i=0; i<scanIndex; i++)
      {
         c = scanBuffer[i];
         if (c=='\r' && scanBuffer[i+1]=='\n') linediv = 2; // crlf detected
         if (c=='\r' || c=='\n' || c==0) break; // enough for us to stop.
      }
      // grab some of the chars starting at the error location - lineending.
      i = 1; j = 0; n = 1;
      // find prev lineending
      while (n<BUFMAX-8 && i<scanIndex) // cap at file start
      {
         c = scanBuffer[scanIndex-i];
         if ((c=='\r' || c=='\n') && i>BUFMAX>>2) break; // at least get a little data
         n++; i++;
      }
      // find next lineending
      while (n<BUFMAX-8 && j<BUFMAX>>1) // cap at half-buf-size forward
      {
         c = scanBuffer[scanIndex+j];
         if (c==0) break;
         if ((c=='\r' || c=='\n') && j>BUFMAX>>2) break; // at least get a little data
         n++; j++;
      }
      if (i) i--; // chop off extra linefeed.
      if (j) j--; // chop off extra linefeed.
      // build our little text block
      if (i) dStrncpy(tempBuf,scanBuffer+scanIndex-i,i);
      dStrncpy(tempBuf+i,"##", 2); // bracketing.
      tempBuf[i+2] = scanBuffer[scanIndex]; // copy the halt character.
      dStrncpy(tempBuf+i+3,"##", 2); // bracketing.
      if (j) dStrncpy(tempBuf+i+5,scanBuffer+scanIndex+1,j); // +1 to go past current char.
      tempBuf[i+j+5] = 0; // null terminate
      for(n=0; n<i+j+5; n++) // convert CR to LF if alone...
         if (tempBuf[n]=='\r' && tempBuf[n+1]!='\n') tempBuf[n] = '\n';
      // write out to console the advanced error report
      Con::warnf(ConsoleLogEntry::Script, ">>> Advanced script error report.  Line %d.", fakeLineIndex);
      Con::warnf(ConsoleLogEntry::Script, ">>> Some error context, with ## on sides of error halt:");
      Con::errorf(ConsoleLogEntry::Script, "%s", tempBuf);
      Con::warnf(ConsoleLogEntry::Script, ">>> Error report complete.\n");
#endif

      // Update the script-visible error buffer.
      const char *prevStr = Con::getVariable("$ScriptError");
      if (prevStr[0])
         dSprintf(tempBuf, sizeof(tempBuf), "%s\n%s Line: %d - Syntax error.", prevStr, fileName, lineIndex);
      else
         dSprintf(tempBuf, sizeof(tempBuf), "%s Line: %d - Syntax error.", fileName, lineIndex);
      Con::setVariable("$ScriptError", tempBuf);

      // We also need to mark that we came up with a new error.
      static S32 sScriptErrorHash=1000;
      Con::setIntVariable("$ScriptErrorHash", sScriptErrorHash++);
   }
   else
      Con::errorf(ConsoleLogEntry::Script, "Syntax error in input.");
}

void SetScanBuffer(const char *sb, const char *fn)
{
   scanBuffer = sb;
   fileName = fn;
   scanIndex = 0;
   lineIndex = 1;
   fakeLineIndex = 1;
}

int CMDgetc()
{
   int ret = scanBuffer[scanIndex];
   if(ret)
      scanIndex++;
   else
      ret = -1;

#ifndef NO_ADVANCED_ERROR_REPORT
   // dhc - added for debuggability of scripts, since compile overhead is large enuf.
   if(ret==0x0a || ret==0x0d) //lf or cr
      fakeLineIndex++;
#endif

   return ret;
}

int CMDwrap()
{
   return 1;
}

static int Sc_ScanVar()
{
   CMDtext[CMDleng] = 0;
	CMDlval.s = StringTable->insert(CMDtext);
	return(VAR);
}
/*
 * Scan string constant.
 */

static int charConv(int in)
{
   switch(in)
   {
      case 'r':
         return '\r';
      case 'n':
         return '\n';
      case 't':
         return '\t';
      default:
         return in;
   }
}

static int getHexDigit(char c)
{
   if(c >= '0' && c <= '9')
      return c - '0';
   if(c >= 'A' && c <= 'F')
      return c - 'A' + 10;
   if(c >= 'a' && c <= 'f')
      return c - 'a' + 10;
   return -1;
}

static int Sc_ScanString(int ret)
{
	CMDtext[CMDleng - 1] = 0;
   if(!collapseEscape(CMDtext+1))
      return -1;
	CMDlval.str = (char *) consoleAllocator.alloc(dStrlen(CMDtext));
   dStrcpy(CMDlval.str, CMDtext + 1);
	return(ret);
}

void expandEscape(char *dest, const char *src)
{
   unsigned char c;
   while((c = (unsigned char) *src++) != 0)
   {
      if(c == '\"')
      {
         *dest++ = '\\';
         *dest++ = '\"';
      }
      else if(c == '\\')
      {
         *dest++ = '\\';
         *dest++ = '\\';
      }
      else if(c == '\r')
      {
         *dest++ = '\\';
         *dest++ = 'r';
      }
      else if(c == '\n')
      {
         *dest++ = '\\';
         *dest++ = 'n';
      }
      else if(c == '\t')
      {
         *dest++ = '\\';
         *dest++ = 't';
      }
      else if(c == '\'')
      {
         *dest++ = '\\';
         *dest++ = '\'';
      }
      else if((c >= 1 && c <= 7) ||
              (c >= 11 && c <= 12) ||
              (c >= 14 && c <= 15))
      {
        /*  Remap around: \b = 0x8, \t = 0x9, \n = 0xa, \r = 0xd */
        static char expandRemap[15] = { 0x0,
                                        0x0,
                                        0x1,
                                        0x2,
                                        0x3,
                                        0x4,
                                        0x5,
                                        0x6,
                                        0x0,
                                        0x0,
                                        0x0,
                                        0x7,
                                        0x8,
                                        0x0,
                                        0x9 };

         *dest++ = '\\';
         *dest++ = 'c';
         if(c == 15)
            *dest++ = 'r';
         else if(c == 16)
            *dest++ = 'p';
         else if(c == 17)
            *dest++ = 'o';
         else 
            *dest++ = expandRemap[c] + '0';
      }
      else if(c < 32)
      {
         *dest++ = '\\';
         *dest++ = 'x';
         S32 dig1 = c >> 4;
         S32 dig2 = c & 0xf;
         if(dig1 < 10)
            dig1 += '0';
         else
            dig1 += 'A' - 10;
         if(dig2 < 10)
            dig2 += '0';
         else
            dig2 += 'A' - 10;
         *dest++ = dig1;
         *dest++ = dig2;
      }
      else
         *dest++ = c;
   }
   *dest = '\0';
}   

bool collapseEscape(char *buf)
{
   int len = dStrlen(buf) + 1;
   for(int i = 0; i < len;)
   {
      if(buf[i] == '\\')
      {
         if(buf[i+1] == 'x')
         {
            int dig1 = getHexDigit(buf[i+2]);
            if(dig1 == -1)
               return false;

            int dig2 = getHexDigit(buf[i+3]);
            if(dig2 == -1)
               return false;
            buf[i] = dig1 * 16 + dig2;
            dMemmove(buf + i + 1, buf + i + 4, len - i - 3);
            len -= 3;
            i++;
         }
         else if(buf[i+1] == 'c')
         {
            /*  Remap around: \t = 0x9, \n = 0xa, \r = 0xd */
            static char collapseRemap[10] = { 0x1,
                                              0x2,
                                              0x3,
                                              0x4,
                                              0x5,
                                              0x6,
                                              0x7,
                                              0xb,
                                              0xc,
                                              0xe };
                                            
            if(buf[i+2] == 'r')
                buf[i] = 15;
            else if(buf[i+2] == 'p')
               buf[i] = 16;
            else if(buf[i+2] == 'o')
               buf[i] = 17;
            else
            {
                int dig1 = buf[i+2] - '0';
                if(dig1 < 0 || dig1 > 9)
                   return false;
                buf[i] = collapseRemap[dig1];
            }
            // Make sure we don't put 0x1 at the beginning of the string.
            if ((buf[i] == 0x1) && (i == 0))
            {
               buf[i] = 0x2;
               buf[i+1] = 0x1;
               dMemmove(buf + i + 2, buf + i + 3, len - i - 1);
               len -= 1;
            }
            else
            {
               dMemmove(buf + i + 1, buf + i + 3, len - i - 2);
               len -= 2;
            }
            i++;         
         }
         else
         {
            buf[i] = charConv(buf[i+1]);
            dMemmove(buf + i + 1, buf + i + 2, len - i - 1);
            len--;
            i++;
         }
      }
      else
         i++;
   }
   return true;
}

static int Sc_ScanNum()
{
   CMDtext[CMDleng] = 0;
	CMDlval.f = atof(CMDtext);
	return(FLTCONST);
}

static int Sc_ScanHex()
{
   int val = 0;
   dSscanf(CMDtext, "%x", &val);
   CMDlval.i = val;
   return INTCONST;
}


