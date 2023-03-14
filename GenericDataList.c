/*
htop - GenericDataList.h
(C) 2022 Sohaib Mohammed
(C) 2022-2023 htop dev team
(C) 2022-2023 Red Hat, Inc.
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "GenericDataList.h"

#include "Object.h"


GenericDataList* GenericDataList_new(Settings* settings) {
   return GenericDataList_addPlatformList(settings);
}

void GenericDataList_delete(GenericDataList* this) {
   if (this)
      GenericDataList_removePlatformList(this);
}

void GenericDataList_addGenericData(GenericDataList* this, GenericData* g) {
   Vector_add(this->genericDataRow, g);
   Hashtable_put(this->genericDataTable, this->totalRows, g);

   this->totalRows++;
}

void GenericDataList_removeGenericData(GenericDataList* this) {
   int idx = this->totalRows - 1;
   Object* last = Vector_get(this->genericDataRow, idx);

   GenericData_delete(last);

   Vector_remove(this->genericDataRow, idx);
   Hashtable_remove(this->genericDataTable, idx);

   this->totalRows--;
}

GenericData* GenericDataList_getGenericData(GenericDataList* this, GenericData_New constructor) {
   GenericData* g = constructor(this->settings);

   return g;
}

void GenericDataList_scan(GenericDataList* this, bool pauseUpdate) {
   GenericDataList_goThroughEntries(this, pauseUpdate);
}

void GenericDataList_setPanel(GenericDataList* this, Panel* panel) {
   this->panel = panel;
}

static void GenericDataList_updateDisplayList(GenericDataList* this) {
   if (this->needsSort)
      Vector_insertionSort(this->genericDataRow);
   Vector_prune(this->displayList);
   int size = Vector_size(this->genericDataRow);
   for (int i = 0; i < size; i++)
      Vector_add(this->displayList, Vector_get(this->genericDataRow, i));
   this->needsSort = false;
}

GenericField GenericDataList_keyAt(const GenericDataList* this, int at) {
   int x = 0;
   const Settings* settings = this->settings;
   const GenericField* fields = settings->ss->fields;
   GenericField field;
   for (int i = 0; (field = fields[i]); i++) {
      int len = strlen(ProcessField_alignedTitle(settings, field));
      if (at >= x && at <= x + len) {
         return field;
      }
      x += len;
   }
   return NULL_PROCESSFIELD;
}

void GenericDataList_printHeader(const GenericDataList* this, RichString* header) {
   RichString_rewind(header, RichString_size(header));

   const Settings* settings = this->settings;
   const ScreenSettings* ss = settings->ss;
   const GenericField* fields = ss->fields;

   GenericField key = ScreenSettings_getActiveSortKey(ss);

   for (int i = 0; fields[i]; i++) {
      int color;
      if (key == fields[i]) {
         color = CRT_colors[PANEL_SELECTION_FOCUS];
      } else {
         color = CRT_colors[PANEL_HEADER_FOCUS];
      }

      RichString_appendWide(header, color, ProcessField_alignedTitle(settings, fields[i]));
      if (key == fields[i] && RichString_getCharVal(*header, RichString_size(header) - 1) == ' ') {
         bool ascending = ScreenSettings_getActiveDirection(ss) == 1;
         RichString_rewind(header, 1);  // rewind to override space
         RichString_appendnWide(header,
                                CRT_colors[PANEL_SELECTION_FOCUS],
                                CRT_treeStr[ascending ? TREE_STR_ASC : TREE_STR_DESC],
                                1);
      }
   }
}

void GenericDataList_rebuildPanel(GenericDataList* this) {
   GenericDataList_updateDisplayList(this);
   const int genericDataCount = Vector_size(this->displayList);
   int idx = 0;

   for (int i = 0; i < genericDataCount; i++) {
      GenericData* g = (GenericData*) Vector_get(this->displayList, i);

      Panel_set(this->panel, idx, (Object*)g);

      idx++;
   }
}
