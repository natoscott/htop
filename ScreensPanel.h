#ifndef HEADER_ScreensPanel
#define HEADER_ScreensPanel
/*
htop - ScreensPanel.h
(C) 2004-2011 Hisham H. Muhammad
(C) 2020-2022 htop dev team
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include <stdbool.h>

#include "AvailableColumnsPanel.h"
#include "ColumnsPanel.h"
#include "DynamicScreen.h"
#include "ListItem.h"
#include "Object.h"
#include "Panel.h"
#include "ScreenManager.h"
#include "Settings.h"

#ifndef SCREEN_NAME_LEN
#define SCREEN_NAME_LEN 20
#endif

typedef struct ActiveScreensPanel_ {
   Panel super;

   ScreenManager* scr;
   Settings* settings;
   ColumnsPanel* activeColumns;
   AvailableColumnsPanel* availableColumns;
   char buffer[SCREEN_NAME_LEN + 1];
   char* saved;
   int cursor;
   bool moving;
   ListItem* renamingItem;
} ActiveScreensPanel;

typedef struct ActiveScreenListItem_ {
   ListItem super;
   ScreenSettings* ss;
} ActiveScreenListItem;

typedef struct ScreensPanel_ {
   Panel super;

   ScreenManager* scr;
   Settings* settings;
   ActiveScreensPanel* activeScreens;
   int cursor;
} ScreensPanel;

typedef struct ScreenListItem_ {
   ListItem super;
   DynamicScreen* ds;
} ScreenListItem;


ScreensPanel* ScreensPanel_new(Settings* settings);

extern ObjectClass ActiveScreenListItem_class;

ActiveScreenListItem* ActiveScreenListItem_new(const char* value, ScreenSettings* ss);

extern PanelClass ActiveScreensPanel_class;

ActiveScreensPanel* ActiveScreensPanel_new(Settings* settings);

#endif
