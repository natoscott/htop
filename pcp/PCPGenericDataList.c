/*
htop - PCPGenericDataList.c
(C) 2022 Sohaib Mohammed
(C) 2022 htop dev team
(C) 2022 Red Hat, Inc.
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "pcp/PCPGenericDataList.h"

#include <stddef.h>
#include <stdlib.h>

#include "DynamicColumn.h"
#include "GenericData.h"
#include "GenericDataList.h"
#include "Hashtable.h"
#include "Object.h"
#include "Process.h"
#include "Settings.h"
#include "Vector.h"
#include "XUtils.h"

#include "pcp/PCPDynamicColumn.h"
#include "pcp/PCPGenericData.h"
#include "pcp/PCPMetric.h"


static int getColumnCount(const ProcessField* fields) {
   int count = 0;
   for (unsigned int i = 0; fields[i]; i++)
      count = i;

   return count + 1;
}

static int getRowCount(int keyMetric) {
   int count = PCPMetric_instanceCount(keyMetric);
   return (count > 0) ? count : 0;
}

static void allocRows(GenericDataList* gl, int requiredRows) {
   GenericData* g;
   int currentRowsCount = Vector_size(gl->genericDataRow);

   if (currentRowsCount != requiredRows) {
      if (currentRowsCount > requiredRows) {
         for (int r = currentRowsCount; r > requiredRows; r--) {
            int idx = r - 1;

            PCPGenericData* gg = (PCPGenericData*)Vector_get(gl->genericDataRow, idx);
            PCPGenericData_removeAllFields(gg);
            GenericDataList_removeGenericData(gl);
         }
      }
      if (currentRowsCount < requiredRows) {
         for (int r = currentRowsCount; r < requiredRows; r++) {
            g = GenericDataList_getGenericData(gl, PCPGenericData_new);
            GenericDataList_addGenericData(gl, g);
         }
      }
   }
}

static void allocColumns(GenericDataList* gl, int requiredColumns, int requiredRows) {
   PCPGenericData* firstrow = (PCPGenericData*)Vector_get(gl->genericDataRow, 0);
   int currentColumns = firstrow->fieldsCount;

   if (currentColumns < requiredRows) {
      for (int r = 0; r < Vector_size(gl->genericDataRow); r++) {
         PCPGenericData* gg = (PCPGenericData*)Vector_get(gl->genericDataRow, r);
         for (int c = gg->fieldsCount; c < requiredColumns; c++)
            PCPGenericData_addField(gg);
      }
   }
   if (currentColumns > requiredRows) {
      for (int r = 0; r < Vector_size(gl->genericDataRow); r++) {
         PCPGenericData* gg = (PCPGenericData*)Vector_get(gl->genericDataRow, r);
         for (int c = gg->fieldsCount; c > requiredColumns; c--)
            PCPGenericData_removeField(gg);
      }
   }
}

static int PCPGenericDataList_updateGenericDataList(PCPGenericDataList* this) {
   GenericDataList* gl = (GenericDataList*) this;
   const Settings* settings = gl->settings;
   const ProcessField* fields = settings->ss->fields;
   const PCPDynamicColumn* keyMetric = NULL;
   pmInDom keyInDom = PM_INDOM_NULL;

   if (!settings->ss->dynamic)
      return 0;

   // check if all columns share the same instance domain
   for (unsigned int i = 0; fields[i]; i++) {
      PCPDynamicColumn* dc = Hashtable_get(settings->dynamicColumns, fields[i]);
      if (!keyMetric) {
         keyMetric = dc;
         keyInDom = PCPMetric_InDom(dc->id);
      } else if (!dc) {
         return -1;
      } else if (keyInDom != PCPMetric_InDom(dc->id)) {
         return 0;
      }
   }
   if (!keyMetric) {
      fprintf(stderr, "Error: no configuration for screen '%s'\n", settings->ss->dynamic);
      return -1;
   }

   int requiredColumns = getColumnCount(fields);
   int requiredRows = getRowCount(keyMetric->id);

   // allocate memory
   allocRows(gl, requiredRows);
   if (requiredRows == 0)
      return 0;
   allocColumns(gl, requiredColumns, requiredRows);

   // fill instance list
   int instance = -1, offset = -1;
   while (PCPMetric_iterate(keyMetric->id, &instance, &offset)) {
      for (unsigned int i = 0; fields[i]; i++) {
         PCPDynamicColumn* column = Hashtable_get(settings->dynamicColumns, fields[i]);
         if (!column)
            return -1;

         const pmDesc* desc = PCPMetric_desc(column->id);

         pmAtomValue value;
         if (PCPMetric_instance(column->id, instance, offset, &value, desc->type)) {
            GenericData* g = Hashtable_get(gl->genericDataTable, offset);
            PCPGenericData* gg = (PCPGenericData*) g;
            if (!gg)
               return -1;
            gg->offset = offset;

            PCPGenericDataField* field = (PCPGenericDataField*) Hashtable_get(gg->fields, i);
            if (!field)
               return -1;
            field->id = column->id;
            field->desc = desc;
            field->value = value;
            field->offset = offset;
            field->instance = instance;
         }
      }
   }

   return 0;
}

void GenericDataList_goThroughEntries(GenericDataList* super, bool pauseUpdate)
{
   if (!pauseUpdate) {
      PCPGenericDataList* this = (PCPGenericDataList*) super;
      PCPGenericDataList_updateGenericDataList(this);
   }
}

GenericDataList* GenericDataList_addPlatformList(Settings* settings)
{
   PCPGenericDataList* this = xCalloc(1, sizeof(PCPGenericDataList));
   GenericDataList* super = &(this->super);

   super->genericDataRow = Vector_new(Class(GenericData), false, DEFAULT_SIZE);
   super->displayList = Vector_new(Class(GenericData), false, DEFAULT_SIZE);
   super->genericDataTable = Hashtable_new(200, false);

   super->totalRows = 0;
   super->needsSort = true;
   super->settings = settings;

   return super;
}

void GenericDataList_removePlatformList(GenericDataList* gl)
{
   // nathans TODO
   // SMA FIXME loop & GenericDataList_removeGenericData()
   Hashtable_delete(gl->genericDataTable);
   Vector_delete(gl->genericDataRow);

   PCPGenericDataList* this = (PCPGenericDataList*) gl;
   free(this);
}
