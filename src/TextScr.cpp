#include "TextScr.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <ctime>

#include "WindowsWrapper.h"


#include "Backends/Misc.h"
#include "ArmsItem.h"
#include "Boss.h"
#include "BossLife.h"
#include "CommonDefines.h"
#include "Draw.h"
#include "Ending.h"
#include "Escape.h"
#include "Fade.h"
#include "Flags.h"
#include "Flash.h"
#include "Frame.h"
#include "Game.h"
#include "Generic.h"
#include "KeyControl.h"
#include "Main.h"
#include "Map.h"
#include "MapName.h"
#include "MiniMap.h"
#include "MyChar.h"
#include "MycParam.h"
#include "NpChar.h"
#ifdef EXTRA_SOUND_FORMATS
#include "ExtraSoundFormats.h"
#endif
#include "Organya.h"
#include "Profile.h"
#include "SelStage.h"
#include "Sound.h"
#include "Stage.h"

#define TEXT_LEFT (WINDOW_WIDTH / 2 - 108)

#define IS_COMMAND(c1, c2, c3) (gTS.data[gTS.p_read + 1] == (c1) && gTS.data[gTS.p_read + 2] == (c2) && gTS.data[gTS.p_read + 3] == (c3))

TEXT_SCRIPT gTS;

static char text[4][0x40];

const RECT gRect_line = {0, 0, 216, 16};

#ifdef FIX_BUGS
static unsigned long nod_color;
#endif

unsigned int cion = 0;
unsigned int gMIMCurrentNum = 0;

using namespace std;

// Initialize and end tsc
BOOL InitTextScript2(void)
{
	int i;

#ifdef FIX_BUGS
	nod_color = GetCortBoxColor(RGB(0xFF, 0xFF, 0xFE));
#endif

	// Clear flags
	gTS.mode = 0;
	g_GameFlags &= ~4;

	// Create line surfaces
	for (i = 0; i < 4; ++i)
		MakeSurface_Generic(gRect_line.right, gRect_line.bottom, (SurfaceID)(SURFACE_ID_TEXT_LINE1 + i), FALSE);

	// Clear text
	memset(text, 0, sizeof(text));

	return TRUE;
}

void EndTextScript(void)
{
	int i;

	// Free TSC buffer
	free(gTS.data);

	// Release buffers
	ReleaseSurface(SURFACE_ID_TEXT_BOX);

	for (i = 0; i < 4; ++i)
		ReleaseSurface((SurfaceID)(SURFACE_ID_TEXT_LINE1 + i));
}

// Decrypt .tsc
void EncryptionBinaryData2(unsigned char *pData, long size)
{
	int i;
	int work;

	int half;
	int val1;

	half = size / 2;

	if (pData[half] == 0)
		val1 = -7;
	else
		val1 = (pData[half] % 0x100) * -1;

	for (i = 0; i < size; ++i)
	{
		work = pData[i];
		work += val1;

		if (i != half)
			pData[i] = work % 0x100;
	}
}

// Load generic .tsc
BOOL LoadTextScript2(const char *name)
{
	FILE *fp;
	std::string path;

	// Get path
	path = gDataPath + '/' + name;

	gTS.size = GetFileSizeLong(path.c_str());
	if (gTS.size == -1)
		return FALSE;

	free(gTS.data);
	gTS.data = (char*)malloc(gTS.size + 1);
	if (gTS.data == NULL)
		return FALSE;

	// Open file
	fp = fopen(path.c_str(), "rb");
	if (fp == NULL)
		return FALSE;

	// Read data
	fread(gTS.data, 1, gTS.size, fp);
	gTS.data[gTS.size] = '\0';
	fclose(fp);

	// Set path
	gTS.path = name;

	// Decrypt data
	EncryptionBinaryData2((unsigned char*)gTS.data, gTS.size);

	return TRUE;
}

// Load stage .tsc
BOOL LoadTextScript_Stage(const char *name)
{
	FILE *fp_head, *fp_body;
	std::string path;
	long head_size;
	long body_size;

	// Open Head.tsc and the stage's .tsc
	path = gDataPath + "/Head.tsc";

	head_size = GetFileSizeLong(path.c_str());
	if (head_size == -1)
		return FALSE;

	fp_head = fopen(path.c_str(), "rb");
	if (fp_head == NULL)
		return FALSE;

	path = gDataPath + '/' + name;

	body_size = GetFileSizeLong(path.c_str());
	if (body_size == -1)
		return FALSE;

	fp_body = fopen(path.c_str(), "rb");
	if (fp_body == NULL)
		return FALSE;

	free(gTS.data);
	gTS.data = (char*)malloc(head_size + body_size + 1);
	if (gTS.data == NULL)
		return FALSE;

	// Read Head.tsc
	fread(gTS.data, 1, head_size, fp_head);
	EncryptionBinaryData2((unsigned char*)gTS.data, head_size);
	gTS.data[head_size] = '\0';
	fclose(fp_head);

	fread(&gTS.data[head_size], 1, body_size, fp_body);
	EncryptionBinaryData2((unsigned char*)&gTS.data[head_size], body_size);
	gTS.data[head_size + body_size] = '\0';
	fclose(fp_body);

	// Set parameters
	gTS.size = head_size + body_size;
	gTS.path = name;

	return TRUE;
}

// Get current path
std::string GetTextScriptPath(void)
{
	return gTS.path;
}

// Get 4 digit number from TSC data
int GetTextScriptNo(int a)
{
	int b = 0;
	b += (gTS.data[a++] - '0') * 1000;
	b += (gTS.data[a++] - '0') * 100;
	b += (gTS.data[a++] - '0') * 10;
	b += gTS.data[a] - '0';
	return b;
}

// Start TSC event
BOOL StartTextScript(int no)
{
	//int i;

	// Reset state
	gTS.mode = 1;
	g_GameFlags |= 5;
	gTS.line = 0;
	gTS.p_write = 0;
	gTS.wait = 4;
	gTS.flags = 0;
	gTS.wait_beam = 0;
	gTS.face = 0;
	gTS.item = 0;
	gTS.offsetY = 0;

	gMC.shock = 0;

	gTS.rcText.left = TEXT_LEFT;
	gTS.rcText.top = WINDOW_HEIGHT - 56;
	gTS.rcText.right = WINDOW_WIDTH - TEXT_LEFT;
	gTS.rcText.bottom = gTS.rcText.top + 48;

	/* This is present in the Linux port, but not the Windows version (1.0.0.6, at least)
	// Clear text lines
	for (i = 0; i < 4; ++i)
	{
		gTS.ypos_line[i] = i * 16;
		CortBox2(&gRect_line, 0x000000, (SurfaceID)(SURFACE_ID_TEXT_LINE1 + i));
		memset(text[i], 0, sizeof(text[0]));
	}*/

	// Find where event starts
	gTS.p_read = 0;
	while (1)
	{
		// Check if we are still in the proper range
		if (gTS.data[gTS.p_read] == '\0')
			return FALSE;

		// Check if we are at an event
		if (gTS.data[gTS.p_read] == '#')
		{
			// Check if this is our event
			int event_no = GetTextScriptNo(++gTS.p_read);

			if (no == event_no)
				break;
			if (no < event_no)
				return FALSE;
		}

		++gTS.p_read;
	}

	// Advance until new-line
	while (gTS.data[gTS.p_read] != '\n')
		++gTS.p_read;
	++gTS.p_read;

	return TRUE;
}

BOOL JumpTextScript(int no)
{
	int i;

	// Set state
	gTS.mode = 1;
	g_GameFlags |= 4;
	gTS.line = 0;
	gTS.p_write = 0;
	gTS.wait = 4;
	gTS.wait_beam = 0;

	// Clear text lines
	for (i = 0; i < 4; ++i)
	{
		gTS.ypos_line[i] = i * 16;
		CortBox2(&gRect_line, 0x000000, (SurfaceID)(SURFACE_ID_TEXT_LINE1 + i));
		memset(text[i], 0, sizeof(text[0]));
	}

	// Find where event starts
	gTS.p_read = 0;

	while(1)
	{
		// Check if we are still in the proper range
		if (gTS.data[gTS.p_read] == '\0')
			return FALSE;

		// Check if we are at an event
		if (gTS.data[gTS.p_read] == '#')
		{
			// Check if this is our event
			int event_no = GetTextScriptNo(++gTS.p_read);

			if (no == event_no)
				break;
			if (no < event_no)
				return FALSE;
		}

		++gTS.p_read;
	}

	// Advance until new-line
	while (gTS.data[gTS.p_read] != '\n')
		++gTS.p_read;

	++gTS.p_read;

	return TRUE;
}

// End event
void StopTextScript(void)
{
	// End TSC and reset flags
	gTS.mode = 0;
	g_GameFlags &= ~4;
	g_GameFlags |= 3;
	gTS.flags = 0;
}

// Prepare a new line
void CheckNewLine(void)
{
	if (gTS.ypos_line[gTS.line % 4] == 48)
	{
		gTS.mode = 3;
		g_GameFlags |= 4;
		CortBox2(&gRect_line, 0, (SurfaceID)(SURFACE_ID_TEXT_LINE1 + (gTS.line % 4)));
		memset(text[gTS.line % 4], 0, sizeof(text[0]));
	}
}

int gNumberTextScript[4];

// Type a number into the text buffer
void SetNumberTextScript(int index)
{
	char str[5];
	BOOL bZero;
	int a;
	int b;
	int offset;
	int i;

	// Get digit table
	int table[3];
	table[0] = 1000;
	table[1] = 100;
	table[2] = 10;

	// Get number to print
	a = gNumberTextScript[index];

	bZero = FALSE;
	offset = 0;

	// Trim leading zeroes
	for (i = 0; i < 3; ++i)
	{
		if (a / table[i] || bZero)
		{
			b = a / table[i];
			str[offset] = '0' + (char)b;
			bZero = TRUE;
			a -= b * table[i];
			++offset;
		}
	}

	// Set last digit of string, and add null terminator
	str[offset] = '0' + (char)a;
	str[offset + 1] = '\0';

	// Append number to line
	PutText2(gTS.p_write * 6, 0, str, RGB(0xFF, 0xFF, 0xFE), (SurfaceID)(SURFACE_ID_TEXT_LINE1 + (gTS.line % 4)));
	strcat(text[gTS.line % 4], str);

	// Play sound and reset blinking cursor
	PlaySoundObject(2, SOUND_MODE_PLAY);
	gTS.wait_beam = 0;

	// Check if should move to next line (prevent a memory overflow, come on guys, this isn't a leftover of pixel trying to make text wrapping)
	gTS.p_write += (int)strlen(str);

	if (gTS.p_write >= 35)
	{
		gTS.p_write = 0;
		++gTS.line;
		CheckNewLine();
	}
}

// Clear text lines
void ClearTextLine(void)
{
	int i;

	gTS.line = 0;
	gTS.p_write = 0;
	gTS.offsetY = 0;

	for (i = 0; i < 4; ++i)
	{
		gTS.ypos_line[i] = i * 16;
		CortBox2(&gRect_line, 0x000000, (SurfaceID)(SURFACE_ID_TEXT_LINE1 + i));
		memset(text[i], 0, sizeof(text[0]));
	}
}

// Draw textbox and whatever else
void PutTextScript(void)
{
	int i;
	RECT rect;
	int text_offset;

	if (gTS.mode == 0)
		return;

	if ((gTS.flags & 1) == 0)
		return;

	// Set textbox position
	if (gTS.flags & 0x20)
	{
		gTS.rcText.top = 32;
		gTS.rcText.bottom = gTS.rcText.top + 48;
	}
	else
	{
		gTS.rcText.top = WINDOW_HEIGHT - 56;
		gTS.rcText.bottom = gTS.rcText.top + 48;
	}

	// Draw textbox
	if (gTS.flags & 2)
	{
		RECT rcFrame1 = {0, 0, 244, 8};
		RECT rcFrame2 = {0, 8, 244, 16};
		RECT rcFrame3 = {0, 16, 244, 24};

		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord(gTS.rcText.top - 10), &rcFrame1, SURFACE_ID_TEXT_BOX);
		for (i = 1; i < 7; ++i)
			PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord((i * 8) + gTS.rcText.top - 10), &rcFrame2, SURFACE_ID_TEXT_BOX);
		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) - 122), PixelToScreenCoord((i * 8) + gTS.rcText.top - 10), &rcFrame3, SURFACE_ID_TEXT_BOX);
	}

	// Draw face picture
	RECT rcFace;
	rcFace.left = (gTS.face % 6) * 48;
	rcFace.top = (gTS.face / 6) * 48;
	rcFace.right = rcFace.left + 48;
	rcFace.bottom = rcFace.top + 48;

	if (gTS.face_x < (TEXT_LEFT * 0x200)) 
		gTS.face_x += 0x1000;

	//if (gTS.face_x < (TEXT_LEFT * 0x200))
		//gTS.face_x += 0x1000;
#ifdef FIX_BUGS
	gTS.rcText.top -= 2;
	PutBitmap3(&gTS.rcText, SubpixelToScreenCoord(gTS.face_x), PixelToScreenCoord(gTS.rcText.top), &rcFace, SURFACE_ID_FACE);
	gTS.rcText.top += 2;
#else
	// The top few rows of pixels are cut off by the clip rectangle, and the facepic is off-centre
	PutBitmap3(&gTS.rcText, SubpixelToScreenCoord(gTS.face_x), PixelToScreenCoord(gTS.rcText.top - 3), &rcFace, SURFACE_ID_FACE);
#endif

	// Draw text
	if (gTS.face != 0)
		text_offset = 0;
	else
		text_offset = 0;

	for (i = 0; i < 4; ++i)
		PutBitmap3(&gTS.rcText, PixelToScreenCoord(TEXT_LEFT + text_offset), PixelToScreenCoord(gTS.offsetY + gTS.ypos_line[i] + gTS.rcText.top), &gRect_line, (SurfaceID)(SURFACE_ID_TEXT_LINE1+i));

	// Draw NOD cursor
	if ((gTS.wait_beam++ % 20 > 12) && gTS.mode == 2)
	{
		rect.left = TEXT_LEFT + (gTS.p_write * 6) + text_offset;
		rect.top = gTS.ypos_line[gTS.line % 4] + gTS.rcText.top + gTS.offsetY;
		rect.right = rect.left + 5;
		rect.bottom = rect.top + 11;
#ifdef FIX_BUGS
		CortBox(&rect, nod_color);

		// This is how the Linux port fixed this, but it isn't done
		// the way Pixel would do it (he only calls GetCortBoxColor
		// once, during init functions, so our fix does it that way
		// instead).
		// CortBox(&rect, GetCortBoxColor(RGB(0xFF, 0xFF, 0xFE));
#else
		// This accidentally uses a BGR value directly, without
		// running it though GetCortBoxColor first
		CortBox(&rect, RGB(0xFF, 0xFF, 0xFE));
#endif
	}

	// Draw GIT
	RECT rcItemBox1 = {0, 0, 72, 16};
	RECT rcItemBox2 = {0, 8, 72, 24};
	RECT rcItemBox3 = {240, 0, 244, 8};
	RECT rcItemBox4 = {240, 8, 244, 16};
	RECT rcItemBox5 = {240, 16, 244, 24};

	if (gTS.item != 0)
	{
		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) - 40), PixelToScreenCoord(WINDOW_HEIGHT - 112), &rcItemBox1, SURFACE_ID_TEXT_BOX);
		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) - 40), PixelToScreenCoord(WINDOW_HEIGHT - 96), &rcItemBox2, SURFACE_ID_TEXT_BOX);
		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) + 32), PixelToScreenCoord(WINDOW_HEIGHT - 112), &rcItemBox3, SURFACE_ID_TEXT_BOX);
		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) + 32), PixelToScreenCoord(WINDOW_HEIGHT - 104), &rcItemBox4, SURFACE_ID_TEXT_BOX);
		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) + 32), PixelToScreenCoord(WINDOW_HEIGHT - 96), &rcItemBox4, SURFACE_ID_TEXT_BOX);
		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) + 32), PixelToScreenCoord(WINDOW_HEIGHT - 88), &rcItemBox5, SURFACE_ID_TEXT_BOX);

		if (gTS.item_y < WINDOW_HEIGHT - 104)
			++gTS.item_y;

		if (gTS.item < 1000)
		{
			rect.left = (gTS.item % 32) * 32;
			rect.right = rect.left + 32;
			rect.top = (gTS.item / 16) * 16;
			rect.bottom = rect.top + 16;
			PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) - 16), PixelToScreenCoord(gTS.item_y), &rect, SURFACE_ID_ARMS_IMAGE);
		}
		else
		{
			rect.left = 32 * ((gTS.item - 1000) % 8);
			rect.right = rect.left + 32;
			rect.top = 16 * ((gTS.item - 1000) / 8);
			rect.bottom = rect.top + 16;
			PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) - 20), PixelToScreenCoord(gTS.item_y), &rect, SURFACE_ID_ITEM_IMAGE);
		}
	}

	// Draw Yes / No selection
	RECT rect_yesno = {152, 48, 244, 80};
	RECT rect_cur = {112, 88, 128, 104};

	if (gTS.mode == 6)
	{
		PutBitmap3(&grcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) + 30), PixelToScreenCoord(WINDOW_HEIGHT - 104), &rect_yesno, SURFACE_ID_TEXT_BOX);
		if (gTS.wait == 16)
			PutBitmap3(&grcFull, PixelToScreenCoord((gTS.select * 41) + (WINDOW_WIDTH / 2) + 25), PixelToScreenCoord(WINDOW_HEIGHT - 94), &rect_cur, SURFACE_ID_TEXT_BOX);
	}
}

// Parse TSC
int TextScriptProc(void)
{
	int i;
	char c[3];
	char str[72];
	int w, x, y, z;

	BOOL bExit;

	RECT rcSymbol = {64, 48, 72, 56};

	switch (gTS.mode)
	{
		case 1: // PARSE
			// Type out (faster if ok or cancel are held)
			++gTS.wait;

			if (!(g_GameFlags & 2) && gKey & (gKeyOk | gKeyCancel))
				gTS.wait += 4;

			if (gTS.wait < 4)
				break;

			gTS.wait = 0;

			// Parsing time
			bExit = FALSE;

			while (!bExit)
			{
				if (gTS.data[gTS.p_read] == '<')
				{
					if (IS_COMMAND('E','N','D'))
					{
						gTS.mode = 0;
						gMC.cond &= ~1;
						g_GameFlags |= 3;
						gTS.face = 0;
						bExit = TRUE;
					}
					else if (IS_COMMAND('L','I','+'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						AddLifeMyChar(x);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('M','L','+'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						AddMaxLifeMyChar(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('A','E','+'))
					{
						FullArmsEnergy();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('I','T','+'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						PlaySoundObject(38, SOUND_MODE_PLAY);
						AddItemData(x);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('I','T','-'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SubItemData(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('E','Q','+'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						EquipItem(z, TRUE);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('E','Q','-'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						EquipItem(z, FALSE);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('A','M','+'))
					{
						w = GetTextScriptNo(gTS.p_read + 4);
						x = GetTextScriptNo(gTS.p_read + 9);

						gNumberTextScript[0] = x;
					#ifndef FIX_BUGS
						// z is uninitialised. Probably a leftover from copypasting this from elsewhere.
						gNumberTextScript[1] = z;
					#endif

						PlaySoundObject(38, SOUND_MODE_PLAY);
						AddArmsData(w, x);
						gTS.p_read += 13;
					}
					else if (IS_COMMAND('A','M','-'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SubArmsData(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('Z','A','M'))
					{
						ZeroArmsEnergy_All();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('T','A','M'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						z = GetTextScriptNo(gTS.p_read + 14);
						TradeArms(x, y, z);
						gTS.p_read += 18;
					}
					else if (IS_COMMAND('P','S','+'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						AddPermitStage(x, y);
						gTS.p_read += 13;
					}
					//CION commands
					//Add X amount of cion
					else if (IS_COMMAND('C','I','+'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						cion+= x;
						gTS.p_read += 8;
					}
					//Remove X amount of cion
					else if (IS_COMMAND('C','I','-'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						cion-= x;
						gTS.p_read += 8;
					}
					//Set cion to X (useful for removing all cions!)
					else if (IS_COMMAND('C','I','S'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						cion= x;
						gTS.p_read += 8;
					}
					//If cion is Greater than or Equal to X, go to event Y.
					else if (IS_COMMAND('C','I','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						if (cion >= x)
							JumpTextScript(y);
						else
						gTS.p_read += 13;
					}
					else if (IS_COMMAND('M','P','+'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						SetMapping(x);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('U','N','I'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						ChangeMyUnit(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('S','T','C'))
					{
						SaveTimeCounter();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('T','R','A'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						w = GetTextScriptNo(gTS.p_read + 9);
						x = GetTextScriptNo(gTS.p_read + 14);
						y = GetTextScriptNo(gTS.p_read + 19);

						if (!TransferStage(z, w, x, y))
						{
							#ifdef JAPANESE
							std::string weenie = "ID " + std::to_string(z) + " のステージは存在しないため転送できません。\nPixMap (またはその他の洞窟物語マップ エディター) を使用して、このステージの TextScript を修正してください。" 
							Backend_ShowMessageBox("エラー", weenie.c_str());
							#else
							std::string weenie = "Cannot transfer to stage with the ID " + std::to_string(z) + " because it does not exist.\nPlease use PixMap (or any other Cave Story map editor) to fix the TextScript for this stage.";
							
							Backend_ShowMessageBox("Error", weenie.c_str());
							#endif

							return enum_ESCRETURN_exit;
						}
					}
					else if (IS_COMMAND('M','O','V'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						SetMyCharPosition(x * 0x200 * 0x10, y * 0x200 * 0x10);
						gTS.p_read += 13;
					}
					else if (IS_COMMAND('H','M','C'))
					{
						ShowMyChar(FALSE);
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('S','M','C'))
					{
						ShowMyChar(TRUE);
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('F','L','+'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SetNPCFlag(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('F','L','-'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						CutNPCFlag(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('S','K','+'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SetSkipFlag(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('S','K','-'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						CutSkipFlag(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('K','E','Y'))
					{
						g_GameFlags &= ~2;
						g_GameFlags |= 1;
						gMC.up = FALSE;
						gMC.shock = 0;
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('P','R','I'))
					{
						g_GameFlags &= ~3;
						gMC.shock = 0;
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('F','R','E'))
					{
						g_GameFlags |= 3;
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('N','O','D'))
					{
						gTS.mode = 2;
						gTS.p_read += 4;
						bExit = TRUE;
					}
					else if (IS_COMMAND('C','L','R'))
					{
						ClearTextLine();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('M','S','G'))
					{
						ClearTextLine();
						gTS.flags |= 0x03;
						gTS.flags &= ~0x30;
						if (gTS.flags & 0x40)
							gTS.flags |= 0x10;
						gTS.p_read += 4;
						bExit = TRUE;
					}
					else if (IS_COMMAND('M','S','2'))
					{
						ClearTextLine();
						gTS.flags &= ~0x12;
						gTS.flags |= 0x21;
						if (gTS.flags & 0x40)
							gTS.flags |= 0x10;
						gTS.face = 0;
						gTS.p_read += 4;
						bExit = TRUE;
					}
					else if (IS_COMMAND('M','S','3'))
					{
						ClearTextLine();
						gTS.flags &= ~0x10;
						gTS.flags |= 0x23;
						if (gTS.flags & 0x40)
							gTS.flags |= 0x10;
						gTS.p_read += 4;
						bExit = TRUE;
					}
					else if (IS_COMMAND('M','S','4'))
					{
						ClearTextLine();
						gTS.flags |= 0x01;
						gTS.flags &= ~0x32;
						if (gTS.flags & 0x40)
							gTS.flags |= 0x10;
						gTS.face = 0;
						gTS.p_read += 4;
						bExit = TRUE;
					}
					else if (IS_COMMAND('W','A','I'))
					{
						gTS.mode = 4;
						gTS.wait_next = GetTextScriptNo(gTS.p_read + 4);
						gTS.p_read += 8;
						bExit = TRUE;
					}
					else if (IS_COMMAND('W','A','S'))
					{
						gTS.mode = 7;
						gTS.p_read += 4;
						bExit = TRUE;
					}
					else if (IS_COMMAND('T','U','R'))
					{
						gTS.p_read += 4;
						gTS.flags |= 0x10;
					}
					else if (IS_COMMAND('S','A','T'))
					{
						gTS.p_read += 4;
						gTS.flags |= 0x40;
					}
					else if (IS_COMMAND('C','A','T'))
					{
						gTS.p_read += 4;
						gTS.flags |= 0x40;
					}
					else if (IS_COMMAND('C','L','O'))
					{
						gTS.flags &= ~0x33;
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('E','V','E'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						JumpTextScript(z);
					}
					else if (IS_COMMAND('Y','N','J'))
					{
						gTS.next_event = GetTextScriptNo(gTS.p_read + 4);
						gTS.p_read += 8;
						gTS.mode = 6;
						PlaySoundObject(5, SOUND_MODE_PLAY);
						gTS.wait = 0;
						gTS.select = 0;
						bExit = TRUE;
					}
					else if (IS_COMMAND('F','L','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						z = GetTextScriptNo(gTS.p_read + 9);

						if (GetNPCFlag(x))
							JumpTextScript(z);
						else
							gTS.p_read += 13;
					}
					else if (IS_COMMAND('S','K','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						z = GetTextScriptNo(gTS.p_read + 9);

						if (GetSkipFlag(x))
							JumpTextScript(z);
						else
							gTS.p_read += 13;
					}
					else if (IS_COMMAND('I','T','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						z = GetTextScriptNo(gTS.p_read + 9);

						if (CheckItem(x))
							JumpTextScript(z);
						else
							gTS.p_read += 13;
					}
					else if (IS_COMMAND('A','M','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						z = GetTextScriptNo(gTS.p_read + 9);

						if (CheckArms(x))
							JumpTextScript(z);
						else
							gTS.p_read += 13;
					}
					else if (IS_COMMAND('U','N','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						z = GetTextScriptNo(gTS.p_read + 9);
						if (GetUnitMyChar() == x)
							JumpTextScript(z);
						else
							gTS.p_read += 13;
					}
					else if (IS_COMMAND('E','C','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						z = GetTextScriptNo(gTS.p_read + 9);
						if (GetNpCharAlive(x))
							JumpTextScript(z);
						else
							gTS.p_read += 13;
					}
					else if (IS_COMMAND('N','C','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						z = GetTextScriptNo(gTS.p_read + 9);
						if (IsNpCharCode(x))
							JumpTextScript(z);
						else
							gTS.p_read += 13;
					}
					else if (IS_COMMAND('M','P','J'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						if (IsMapping())
							JumpTextScript(x);
						else
							gTS.p_read += 8;
					}
					else if (IS_COMMAND('S','S','S'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						SetNoise(1, x);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('C','S','S'))
					{
						CutNoise();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('S','P','S'))
					{
					#ifdef FIX_BUGS
						SetNoise(2, 0);
					#else
						// x is not initialised. This bug isn't too bad, since that parameter's not used when the first one is set to 2, but still.
						SetNoise(2, x);
					#endif
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('C','P','S'))
					{
						CutNoise();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('Q','U','A'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SetQuake(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('F','L','A'))
					{
						SetFlash(0, 0, 2);
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('F','A','I'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						StartFadeIn(z);
						gTS.mode = 5;
						gTS.p_read += 8;
						bExit = TRUE;
					}
					else if (IS_COMMAND('F','A','O'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						StartFadeOut(z);
						gTS.mode = 5;
						gTS.p_read += 8;
						bExit = TRUE;
					}
					else if (IS_COMMAND('M','N','A'))
					{
						StartMapName();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('F','O','M'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SetFrameTargetMyChar(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('F','O','N'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						SetFrameTargetNpChar(x, y);
						gTS.p_read += 13;
					}
					else if (IS_COMMAND('F','O','B'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						SetFrameTargetBoss(x, y);
						gTS.p_read += 13;
					}
					else if (IS_COMMAND('S','O','U'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						PlaySoundObject(z, SOUND_MODE_PLAY);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('C','M','U'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						ChangeMusic((MusicID)z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('F','M','U'))
					{
						SetOrganyaFadeout();

						#ifdef EXTRA_SOUND_FORMATS
						ExtraSound_FadeOutMusic();
						#endif

						gTS.p_read += 4;
					}
					else if (IS_COMMAND('R','M','U'))
					{
						ReCallMusic();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('M','L','P'))
					{
						gTS.p_read += 4;
						bExit = TRUE;

						switch (MiniMapLoop())
						{
							case enum_ESCRETURN_exit:
								return enum_ESCRETURN_exit;

							case enum_ESCRETURN_restart:
								return enum_ESCRETURN_restart;
						}
					}
					else if (IS_COMMAND('S','L','P'))
					{
						bExit = TRUE;

						switch (StageSelectLoop(&z))
						{
							case enum_ESCRETURN_exit:
								return enum_ESCRETURN_exit;

							case enum_ESCRETURN_restart:
								return enum_ESCRETURN_restart;
						}

						JumpTextScript(z);
						g_GameFlags &= ~3;
					}
					else if (IS_COMMAND('D','N','P'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						DeleteNpCharEvent(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('D','N','A'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						DeleteNpCharCode(z, TRUE);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('B','O','A'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SetBossCharActNo(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('C','N','P'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						z = GetTextScriptNo(gTS.p_read + 14);
						ChangeNpCharByEvent(x, y, z);
						gTS.p_read += 18;
					}
					else if (IS_COMMAND('A','N','P'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						z = GetTextScriptNo(gTS.p_read + 14);
						SetNpCharActionNo(x, y, z);
						gTS.p_read += 18;
					}
					else if (IS_COMMAND('I','N','P'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						z = GetTextScriptNo(gTS.p_read + 14);
						ChangeCheckableNpCharByEvent(x, y, z);
						gTS.p_read += 18;
					}
					else if (IS_COMMAND('S','N','P'))
					{
						w = GetTextScriptNo(gTS.p_read + 4);
						x = GetTextScriptNo(gTS.p_read + 9);
						y = GetTextScriptNo(gTS.p_read + 14);
						z = GetTextScriptNo(gTS.p_read + 19);
						SetNpChar(w, x * 0x200 * 0x10, y * 0x200 * 0x10, 0, 0, z, NULL, 0x100);
						gTS.p_read += 23;
					}
					else if (IS_COMMAND('M','N','P'))
					{
						w = GetTextScriptNo(gTS.p_read + 4);
						x = GetTextScriptNo(gTS.p_read + 9);
						y = GetTextScriptNo(gTS.p_read + 14);
						z = GetTextScriptNo(gTS.p_read + 19);
						MoveNpChar(w, x * 0x200 * 0x10, y * 0x200 * 0x10, z);
						gTS.p_read += 23;
					}
					else if (IS_COMMAND('S','M','P'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						ShiftMapParts(x, y);
						gTS.p_read += 13;
					}
					else if (IS_COMMAND('C','M','P'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);
						z = GetTextScriptNo(gTS.p_read + 14);
						ChangeMapParts(x, y, z);
						gTS.p_read += 18;
					}
					else if (IS_COMMAND('B','S','L'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);

						if (z != 0)
							StartBossLife(z);
						else
							StartBossLife2();

						gTS.p_read += 8;
					}
					else if (IS_COMMAND('M','Y','D'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SetMyCharDirect(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('M','Y','B'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						BackStepMyChar(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('M','M','0'))
					{
						ZeroMyCharXMove();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('I','N','I'))
					{
						InitializeGame();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('S','V','P'))
					{
						SaveProfile(NULL);
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('L','D','P'))
					{
						if (!LoadProfile(NULL))
							InitializeGame();
					}
					else if (IS_COMMAND('F','A','C'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						if (gTS.face != (signed char)z)	// Not sure why these casts are here, but they're needed for the same assembly code to be produced
						{
							gTS.face = (signed char)z;
							gTS.face_x = (WINDOW_WIDTH / 2 - 17) * 0x300; //gTS.face_x = (WINDOW_WIDTH / 2 - 156) * 0x200;
						}
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('G','I','T'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						gTS.item = z;
						gTS.item_y = WINDOW_HEIGHT - 112;
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('N','U','M'))
					{
						// This supports up to four different values, but only one is actually used (a second is used erroneously)
						z = GetTextScriptNo(gTS.p_read + 4);
						SetNumberTextScript(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('C','R','E'))
					{
						g_GameFlags |= 8;
						StartCreditScript();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('S','I','L'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						SetCreditIllust(z);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('C','I','L'))
					{
						CutCreditIllust();
						gTS.p_read += 4;
					}
					else if (IS_COMMAND('X','X','1'))
					{
						bExit = TRUE;
						z = GetTextScriptNo(gTS.p_read + 4);

						switch (Scene_DownIsland(z))
						{
							case enum_ESCRETURN_exit:
								return enum_ESCRETURN_exit;

							case enum_ESCRETURN_restart:
								return enum_ESCRETURN_restart;
						}

						gTS.p_read += 8;
					}
					else if(IS_COMMAND('D','A','T')) // If date is XXXX:YYYY, run event ZZZZ
					{
						x = GetTextScriptNo(gTS.p_read + 4); // Month
						y = GetTextScriptNo(gTS.p_read + 9); // Day
						z = GetTextScriptNo(gTS.p_read + 14); // Event

						time_t ttime = time(0);
    					tm *local_time = localtime(&ttime);

						cout << local_time->tm_mon;
						cout << local_time->tm_mday;

						if (local_time->tm_mon == x + 1 && local_time->tm_mday == y)
							JumpTextScript(z);
						else
							//gTS.p_read += 18;
							JumpTextScript(z);
					}
					else if (IS_COMMAND('E','V','E'))
					{
						z = GetTextScriptNo(gTS.p_read + 4);
						JumpTextScript(z);
					}
					else if (IS_COMMAND('M','I','M'))
					{
						gMIMCurrentNum = GetTextScriptNo(gTS.p_read + 4);
						gTS.p_read += 8;
					}
					else if (IS_COMMAND('P','H','Y'))
					{
						x = GetTextScriptNo(gTS.p_read + 4);
						y = GetTextScriptNo(gTS.p_read + 9);

						switch (x)
						{
							case 0:
								gMC.physics_normal.max_dash = y;
								break;

							case 1:
								gMC.physics_normal.max_move = y;
								break;

							case 2:
								gMC.physics_normal.gravity1 = y;
								break;

							case 3:
								gMC.physics_normal.gravity2 = y;
								break;

							case 4:
								gMC.physics_normal.dash1 = y;
								break;

							case 5:
								gMC.physics_normal.dash2 = y;
								break;

							case 6:
								gMC.physics_normal.resist = y;
								break;

							case 7:
								gMC.physics_normal.jump = y;
								break;

							case 8:
								gMC.physics_underwater.max_dash = y;
								break;

							case 9:
								gMC.physics_underwater.max_move = y;
								break;

							case 10:
								gMC.physics_underwater.gravity1 = y;
								break;

							case 11:
								gMC.physics_underwater.gravity2 = y;
								break;

							case 12:
								gMC.physics_underwater.dash1 = y;
								break;

							case 13:
								gMC.physics_underwater.dash2 = y;
								break;

							case 14:
								gMC.physics_underwater.resist = y;
								break;

							case 15:
								gMC.physics_underwater.jump = y;
								break;

							case 16:
								gMC.no_splash_or_air_limit_underwater = y;
								break;
						}

						gTS.p_read += 13;
					}
					else if (IS_COMMAND('E','S','C'))
					{
						return enum_ESCRETURN_restart;
					}
					else
					{
						char str_0[0x40];
						#ifdef JAPANESE
						sprintf(str_0, "不明のコード:<%c%c%c", gTS.data[gTS.p_read + 1], gTS.data[gTS.p_read + 2], gTS.data[gTS.p_read + 3]);
						Backend_ShowMessageBox("エラー", str_0);
						#else
						sprintf(str_0, "Unknown code:<%c%c%c", gTS.data[gTS.p_read + 1], gTS.data[gTS.p_read + 2], gTS.data[gTS.p_read + 3]);
						Backend_ShowMessageBox("Error", str_0);
						#endif

						return enum_ESCRETURN_exit;
					}
				}
				else
				{
					if (gTS.data[gTS.p_read] == '\r')
					{
						// Go to new-line
						gTS.p_read += 2;
						gTS.p_write = 0;

						if (gTS.flags & 1)
						{
							++gTS.line;
							CheckNewLine();
						}
					}
					else if (gTS.flags & 0x10)
					{
						// SAT/CAT/TUR printing
						x = gTS.p_read;
						// Break if reaches command, or new-line
						while (gTS.data[x] != '<' && gTS.data[x] != '\r')
						{
							// Skip if SHIFT-JIS
							if (gTS.data[x] & 0x80)
								++x;

							++x;
						}

						// Get text to copy
						y = x - gTS.p_read;
						memcpy(str, &gTS.data[gTS.p_read], y);
						str[y] = '\0';

						gTS.p_write = x;

						// Print text
						PutText2(0, 0, str, RGB(0xFF, 0xFF, 0xFE), (SurfaceID)(SURFACE_ID_TEXT_LINE1 + (gTS.line % 4)));
					#ifdef FIX_BUGS
						strcpy(text[gTS.line % 4], str);
					#else
						sprintf(text[gTS.line % 4], str);	// No point in using an sprintf here, and it makes Clang mad
					#endif

						// Check if should move to next line (prevent a memory overflow, come on guys, this isn't a leftover of pixel trying to make text wrapping)
						gTS.p_read += y;

						if (gTS.p_write >= 35)
							CheckNewLine();

						bExit = TRUE;
					}
					else
					{
						// Get text to print
						c[0] = gTS.data[gTS.p_read];

						if (c[0] & 0x80)
						{
							c[1] = gTS.data[gTS.p_read + 1];
							c[2] = '\0';
						}
						else
						{
							c[1] = '\0';
						}

						// Print text
						if (c[0] == '=')
						{
							Surface2Surface(gTS.p_write * 6, 2, &rcSymbol, (SurfaceID)(SURFACE_ID_TEXT_LINE1 + (gTS.line % 4)), SURFACE_ID_TEXT_BOX);
						}
						else
						{
							PutText2(gTS.p_write * 6, 0, c, RGB(0xFF, 0xFF, 0xFE), (SurfaceID)(SURFACE_ID_TEXT_LINE1 + (gTS.line % 4)));
						}

						strcat(text[gTS.line % 4], c);
						PlaySoundObject(2, SOUND_MODE_PLAY);
						gTS.wait_beam = 0;

						// Offset read and write positions
						if (c[0] & 0x80)
						{
							gTS.p_read += 2;
							gTS.p_write += 2;
						}
						else
						{
							gTS.p_read += 1;
							gTS.p_write += 1;
						}

						if (gTS.p_write >= 35)
						{
							CheckNewLine();
							gTS.p_write = 0;
							++gTS.line;
							CheckNewLine();
						}

						bExit = TRUE;
					}
				}
			}
			break;

		case 2: // NOD
			if (gKeyTrg & (gKeyOk | gKeyCancel))
				gTS.mode = 1;
			break;

		case 3: // NEW LINE
			for (i = 0; i < 4; ++i)
			{
				gTS.ypos_line[i] -= 4;

				if (gTS.ypos_line[i] == 0)
					gTS.mode = 1;

				if (gTS.ypos_line[i] == -16)
					gTS.ypos_line[i] = 48;
			}
			break;

		case 4: // WAI
			if (gTS.wait_next == 9999)
				break;

			if (gTS.wait != 9999)
				++gTS.wait;

			if (gTS.wait < gTS.wait_next)
				break;

			gTS.mode = 1;
			gTS.wait_beam = 0;
			break;

		case 5: // FAI/FAO
			if (GetFadeActive())
				break;

			gTS.mode = 1;
			gTS.wait_beam = 0;
			break;

		case 7: // WAS
			if ((gMC.flag & 8) == 0)
				break;

			gTS.mode = 1;
			gTS.wait_beam = 0;
			break;

		case 6: // YNJ
			if (gTS.wait < 16)
			{
				++gTS.wait;
			}
			else
			{
				// Select option
				if (gKeyTrg & gKeyOk)
				{
					PlaySoundObject(18, SOUND_MODE_PLAY);

					if (gTS.select == 1)
					{
						JumpTextScript(gTS.next_event);
					}
					else
					{
						gTS.mode = 1;
						gTS.wait_beam = 0;
					}
				}
				// Yes
				else if (gKeyTrg & gKeyLeft)
				{
					gTS.select = 0;
					PlaySoundObject(1, SOUND_MODE_PLAY);
				}
				// No
				else if (gKeyTrg & gKeyRight)
				{
					gTS.select = 1;
					PlaySoundObject(1, SOUND_MODE_PLAY);
				}
			}
			break;
	}

	if (gTS.mode == 0)
		g_GameFlags &= ~4;
	else
		g_GameFlags |= 4;

	return enum_ESCRETURN_continue;
}

void RestoreTextScript(void)
{
	int i;

	for (i = 0; i < 4; ++i)
	{
		CortBox2(&gRect_line, 0x000000, (SurfaceID)(SURFACE_ID_TEXT_LINE1 + i));
		PutText2(0, 0, text[i], RGB(0xFF, 0xFF, 0xFE), (SurfaceID)(SURFACE_ID_TEXT_LINE1 + i));
	}
}
