/*
htop - HostnameMeter.c
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h" // IWYU pragma: keep

#include "HostnameMeter.h"

#include <string.h>

#include "CRT.h"
#include "Object.h"
#include "Platform.h"


static const int HostnameMeter_attributes[] = {
   HOSTNAME
};

static void HostnameMeter_updateValues(Meter* this) {
   Platform_getHostname(this->txtBuffer, sizeof(this->txtBuffer));

#ifdef HTOP_PCP
   extern bool Platform_getArchiveMode(void);
   extern void Platform_getArchiveTime(char* buffer, size_t size);
   extern bool Platform_archiveIsPaused(void);

   if (Platform_getArchiveMode()) {
      /* Append archive time and state to hostname */
      char timeBuffer[64];
      Platform_getArchiveTime(timeBuffer, sizeof(timeBuffer));
      bool paused = Platform_archiveIsPaused();
      size_t len = strlen(this->txtBuffer);
      snprintf(this->txtBuffer + len, sizeof(this->txtBuffer) - len,
               " [ARCHIVE: %s%s]", timeBuffer, paused ? " PAUSED" : "");
   }
#endif
}

const MeterClass HostnameMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = HostnameMeter_updateValues,
   .defaultMode = TEXT_METERMODE,
   .supportedModes = (1 << TEXT_METERMODE),
   .maxItems = 0,
   .total = 0.0,
   .attributes = HostnameMeter_attributes,
   .name = "Hostname",
   .uiName = "Hostname",
   .caption = "Hostname: ",
};
