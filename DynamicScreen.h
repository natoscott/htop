#ifndef HEADER_DynamicScreen
#define HEADER_DynamicScreen

#include <stdbool.h>

#include "Hashtable.h"
#include "Panel.h"
#include "Settings.h"


typedef struct DynamicScreen_ {
   char name[32];  /* unique name cannot contain any spaces */
   char* heading;  /* user-settable more readable name */
   char* caption;  /* explanatory text for screen */
   char* fields;
   char* sortKey;
   int direction;
} DynamicScreen;

Hashtable* DynamicScreens_new(void);

void DynamicScreens_delete(Hashtable* dynamics);

void DynamicScreen_done(DynamicScreen* this);

void DynamicScreens_addAvailableColumns(Panel* availableColumns, char* screen);

const char* DynamicScreen_lookup(Hashtable* dynamics, unsigned int key);

bool DynamicScreen_search(Hashtable* dynamics, const char* name, unsigned int* key);

#endif
