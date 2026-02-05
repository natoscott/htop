# Claude Code Session - PCP Archive Replay Implementation

## Project State (2026-02-04)

### Overview
This is htop with Performance Co-Pilot (PCP) backend support. The current implementation supports live metrics only via `pcp-htop`. We are implementing archive replay functionality to allow users to replay historical PCP archive data, similar to how `pcp-atop` works.

### Current PCP Integration

The codebase already has extensive PCP integration:

- **Entry point**: `pcp-htop.c` - calls `pmSetProgname()` and `pmGetOptions()` to handle PCP options
- **Platform abstraction**: `pcp/Platform.c` and `pcp/Platform.h` - main PCP integration layer
- **Metrics handling**: `pcp/Metric.c` and `pcp/Metric.h` - wrapper around PMAPI for metric fetch/values
- **Machine state**: `pcp/PCPMachine.c` - handles CPU, memory, and system metrics
- **Process table**: `pcp/PCPProcessTable.c` - handles per-process metrics
- **Dynamic config**: `pcp/columns/`, `pcp/meters/`, `pcp/screens/` - configuration files for custom metrics

### Archive Support Status

**Currently Working:**
- Archives CAN be loaded via `pcp -a <archive> htop` (uses `pmGetOptions()`)
- The code in `Platform_init()` (pcp/Platform.c:321-362) already detects `PM_CONTEXT_ARCHIVE`
- Archive time offset is calculated and stored in `pcp->offset` (timeval)
- Initial archive position is set based on `opts.start`

**Not Working (What We're Implementing):**
- No interactive navigation through archive time
- No UI controls to move forward/backward in time
- No pause/resume functionality
- No display of current archive position
- No way to jump to specific time
- No sampling interval control for archives

### PCP Archive Locations for Testing

1. `/home/nathans/git/pcp/qa/archives/` - QA test archives
2. `/var/log/pcp/pmlogger/fedora/` - Local pmlogger archives (current: 20260204.00.13)

### Key PMAPI Functions for Archive Replay

- `pmSetMode()` - Navigate to specific time position in archive
- `pmFetch()` - Fetch metrics at current position (already used)
- `pmGetArchiveLabel()` - Get archive metadata (start/end times)
- `pmGetArchiveEnd()` - Get archive end time
- Result timestamp in `pcp->result->timestamp` (struct timespec in PMAPI v3+, timeval in older)

### Architecture Notes

**htop Key Components:**
- `Action.h/c` - Keyboard action handlers
- `FunctionBar.h/c` - F1-F10 function key bar at bottom
- `State` struct - Global state (machine, mainPanel, header, pauseUpdate, etc.)
- `Htop_Reaction` - Return values for actions (HTOP_REFRESH, HTOP_REDRAW_BAR, etc.)

**Current Key Bindings:**
- F1-F10 already assigned (Help, Setup, Search, Filter, Tree, Sort, Nice+, Nice-, Kill, Quit)
- Space: Pause/unpause updates (`st->pauseUpdate`)
- Arrow keys, Page Up/Down: Process list navigation

### Implementation Plan

See `ARCHIVE_REPLAY_PLAN.md` for detailed implementation checklist.

### Development Notes

- Use `struct timespec` for PCP timestamps (native PCP format), not `struct timeval`
- PMAPI version 3+ uses `timespec`, older uses `timeval` (check `PMAPI_VERSION`)
- Archive mode should disable live-only features (e.g., `Metric_enableThreads()`)
- Function bar labels should change in archive mode to show navigation controls
- Need visual indicator in header showing: archive mode, current time, paused state

### Testing Command

```bash
# Test with local archive
pcp -a /var/log/pcp/pmlogger/fedora/20260204.00.13 htop

# Test with QA archive
pcp -a /home/nathans/git/pcp/qa/archives/20180102 htop
```

### Build Commands

```bash
./autogen.sh && ./configure --enable-pcp && make
```

### Important Files Modified (or to be modified)

- `pcp/Platform.h` - Add archive state to Platform struct
- `pcp/Platform.c` - Archive initialization, navigation functions
- `pcp/Metric.c` - Update `Metric_fetch()` for archive navigation
- `Action.h/c` - Add archive navigation action handlers
- `Header.c` - Add archive time display (if needed)
- `MainPanel.c` or main loop - Update function bar in archive mode

### Design Decisions to Make

1. **Key bindings for archive navigation:**
   - Option A: Reuse existing keys (Left/Right for time, Space for pause)
   - Option B: Use function keys (F11/F12 or reassign F7-F9 in archive mode)
   - Option C: Use letter keys (B=back, F=forward, P=pause, J=jump)

2. **Time navigation increments:**
   - Single sample step vs. time-based (1 min, 5 min, 1 hour)
   - Should it be configurable?

3. **Archive end behavior:**
   - Stop at end, wrap around, or automatically reverse?

4. **Display format:**
   - Show absolute time or relative to start/current?
   - Show in header or function bar area?

### Next Steps

1. Create detailed implementation plan with checkboxes
2. Implement archive state tracking in Platform struct
3. Implement archive navigation functions (pmSetMode wrappers)
4. Add action handlers for navigation keys
5. Update UI to show archive state and controls
6. Test with real archives
