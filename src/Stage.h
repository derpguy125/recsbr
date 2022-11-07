#pragma once

#include "WindowsWrapper.h"

typedef enum MusicID
{
	MUS_SILENCE = 0x0,
	MUS_CURLY = 0x1,
	MUS_GESTATION = 0x2,
	MUS_GAME_OVER = 0x3,
	MUS_GRAVITY = 0x4,
	MUS_SAFETY = 0x5,
	MUS_PIER_WALK = 0x6,
	MUS_GEOTHERMAL = 0x7,
	MUS_PEOPLE_OF_THE_ROOT = 0x8,
	MUS_LIVING_WATERWAY = 0x9,
	MUS_MELTDOWN = 0xA,
	MUS_MOONSONG = 0xB,
	MUS_PLANT = 0xC,
	MUS_REQUIEM = 0xD,
	MUS_ROCKORG = 0xE,
	MUS_WHITE = 0xF,
	MUS_OPPRESSION = 0x10,
	
} MusicID;

typedef struct STAGE_TABLE
{
	char parts[0x20];
	char map[0x20];
	int bkType;
	char back[0x20];
	char npc[0x20];
	char boss[0x20];
	signed char boss_no;
	char name[0x22];
} STAGE_TABLE;

extern int gStageNo;
extern MusicID gMusicNo;
extern unsigned int gOldPos;
extern MusicID gOldNo;

extern const STAGE_TABLE *gTMT;

BOOL LoadStageTable();
BOOL TransferStage(int no, int w, int x, int y);
void ChangeMusic(MusicID no);
void ReCallMusic(void);
