# PCP Archive Replay Implementation Plan

## Phase 1: Core Infrastructure

### 1.1 Platform State Tracking
- [ ] Add archive state fields to Platform struct in `pcp/Platform.h`:
  - `struct timespec archiveStart` - archive start timestamp
  - `struct timespec archiveEnd` - archive end timestamp
  - `struct timespec archiveCurrent` - current position in archive
  - `int replayMode` - pmSetMode mode: PM_MODE_FORW or PM_MODE_BACK
  - `struct timeval delta` - time delta for PM_MODE_FORW/BACK navigation
  - `bool paused` - if true, don't advance, re-fetch at archiveCurrent

Note: Use `opts.context` (PM_CONTEXT_ARCHIVE vs PM_CONTEXT_HOST/LOCAL) to detect archive mode, not a separate bool.

### 1.2 Archive Detection and Initialization
- [ ] Update `Platform_init()` in `pcp/Platform.c`:
  - [ ] When `opts.context == PM_CONTEXT_ARCHIVE`:
    - [ ] Call `pmGetArchiveLabel()` to get archive start time
    - [ ] Call `pmGetArchiveEnd()` to get archive end time
    - [ ] Initialize `archiveCurrent` from `opts.start` (if set) or archive start
    - [ ] Set initial `paused = true` (start paused)
    - [ ] Set `replayMode = PM_MODE_FORW` (default forward direction when unpaused)
    - [ ] Initialize `delta` to default step size (e.g., zero for single sample step)
  - [ ] For live contexts (HOST/LOCAL), these fields remain unused/zero

### 1.3 Archive Navigation Functions
Add new functions to `pcp/Platform.c`:

- [ ] `Platform_getArchiveMode()` - returns bool, checks `opts.context == PM_CONTEXT_ARCHIVE`
- [ ] `Platform_getArchiveTime(char* buffer, size_t size)` - format current archive time
- [ ] `Platform_getArchiveBounds(struct timespec* start, struct timespec* end)` - get time range
- [ ] `Platform_archiveStepForward(int samples)` - advance N samples forward
- [ ] `Platform_archiveStepBackward(int samples)` - go back N samples
- [ ] `Platform_archiveSeekTime(struct timespec* target)` - jump to specific time
- [ ] `Platform_archiveTogglePause()` - toggle paused flag
- [ ] `Platform_archiveSetMode(int mode)` - set replay mode (PM_MODE_FORW or PM_MODE_BACK)

Declare these in `pcp/Platform.h`.

## Phase 2: Metric Fetching for Archives

### 2.1 Update Metric_fetch()
Modify `Metric_fetch()` in `pcp/Metric.c`:

- [ ] Check if `opts.context == PM_CONTEXT_ARCHIVE`
- [ ] If archive mode and paused:
  - [ ] Use `pmSetMode()` to position at `pcp->archiveCurrent` and fetch that position
- [ ] If archive mode and NOT paused:
  - [ ] Call `pmSetMode(pcp->replayMode, &pcp->delta, 0)` before `pmFetch()`
  - [ ] Delta determines step size (0 = one sample, >0 = time-based step)
  - [ ] After successful fetch, update `pcp->archiveCurrent` from `pcp->result->timestamp`
- [ ] Handle archive end conditions:
  - [ ] If PM_ERR_EOL (end of log): set paused = true
  - [ ] If going backward past start: set paused = true

### 2.2 Archive End Detection
- [ ] After `pmFetch()`, check for PM_ERR_EOL error
- [ ] If at end going forward (PM_MODE_FORW): set paused = true
- [ ] If at start going backward (PM_MODE_BACK): set paused = true

## Phase 3: User Interface Integration

### 3.1 Archive Action Handlers
Add new actions to `Action.c`:

- [ ] `actionArchiveStepForward` - move forward one sample (PM_MODE_FORW, delta=0)
- [ ] `actionArchiveStepBackward` - move backward one sample (PM_MODE_BACK, delta=0)
- [ ] `actionArchiveJumpForward` - jump forward 5 minutes (PM_MODE_FORW, delta=5min)
- [ ] `actionArchiveJumpBackward` - jump backward 5 minutes (PM_MODE_BACK, delta=5min)
- [ ] `actionArchiveTogglePause` - toggle paused flag
- [ ] `actionArchiveSeekStart` - jump to archive start
- [ ] `actionArchiveSeekEnd` - jump to archive end
- [ ] `actionArchiveSeekTime` - interactive time seek dialog

Declare these in `Action.h`.

### 3.2 Key Bindings
In `Action_setBindings()` (Action.c):

When in archive mode, assign keys:
- [ ] Left Arrow: step backward
- [ ] Right Arrow: step forward
- [ ] Ctrl+Left: jump backward 5 min
- [ ] Ctrl+Right: jump forward 5 min
- [ ] Space: toggle pause (reuse existing pause mechanism)
- [ ] Home: seek to start
- [ ] End: seek to end
- [ ] 'j' or '/': seek to time dialog

### 3.3 Function Bar Updates
Modify function bar display when in archive mode:

- [ ] Update `MainPanel.c` or main event loop to detect archive mode
- [ ] Change function bar labels for archive mode:
  - F1: Help (keep)
  - F2: Setup (keep)
  - F3: Pause (toggle paused flag)
  - F4: <<< (jump back)
  - F5: < (step back)
  - F6: > (step forward)
  - F7: >>> (jump forward)
  - F8: Start (seek to start)
  - F9: End (seek to end)
  - F10: Quit (keep)

### 3.4 Archive Time Display
Add archive time indicator to header:

Option A: Add to Header.c
- [ ] Modify `Header_draw()` to show archive mode indicator
- [ ] Display: `[ARCHIVE] 2026-02-04 16:55:23 [PAUSED]` or similar
- [ ] Use color to indicate paused (yellow) vs playing (green)

Option B: Add to function bar area
- [ ] Use `FunctionBar_drawExtra()` to show time in function bar
- [ ] Format: `Archive: 16:55:23 / 17:30:45 [PAUSED]`

## Phase 4: Enhanced Features (Optional)

### 4.1 Playback Speed Control
- [ ] Add `archiveSpeed` field (1.0 = normal, 2.0 = 2x, 0.5 = half speed)
- [ ] Add actions to increase/decrease speed
- [ ] Adjust delta time intervals accordingly

### 4.2 Time Seek Dialog
- [ ] Create interactive dialog for time input
- [ ] Support formats: HH:MM:SS, absolute timestamp, relative (+5m, -1h)
- [ ] Show archive time bounds in dialog

### 4.3 Sample Interval Display
- [ ] Show time between samples in archive
- [ ] Option to skip to next/previous hour/day boundary

### 4.4 Archive Info Display
- [ ] Show archive metadata (F2 setup or help screen)
- [ ] Hostname, archive creation time, duration, sample count

## Phase 5: Testing Infrastructure

### 5.1 Archive Replay Unit Testing
- [ ] Test archive mode detection
- [ ] Test forward navigation (PM_MODE_FORW)
- [ ] Test backward navigation (PM_MODE_BACK)
- [ ] Test pause/resume (paused flag)
- [ ] Test seek to boundaries
- [ ] Test archive end behavior (PM_ERR_EOL)

### 5.2 Archive Replay Integration Testing
- [ ] Test with QA archives: `/home/nathans/git/pcp/qa/archives/20180102`
- [ ] Test with local archives: `/var/log/pcp/pmlogger/fedora/20260204.00.13`
- [ ] Test with multi-volume archives
- [ ] Test with corrupted/truncated archives
- [ ] Test with very large archives

### 5.3 Archive Replay Edge Cases
- [ ] Single-sample archive
- [ ] Archive with gaps
- [ ] Archive spanning multiple days
- [ ] Very old archive formats

### 5.4 Deterministic Regression Test Framework

#### 5.4.1 Test Archive Creation
- [ ] Create dedicated test archives in `qa/archives/` directory:
  - [ ] `htop-basic.tar.gz` - 5 minutes, stable system load, known process set
  - [ ] `htop-cpu-stress.tar.gz` - CPU intensive workload, varying process counts
  - [ ] `htop-memory-test.tar.gz` - Memory allocation patterns, swap usage
  - [ ] `htop-io-test.tar.gz` - Disk I/O activity, network traffic
  - [ ] `htop-process-lifecycle.tar.gz` - Process creation/termination events
  - [ ] `htop-minimal.tar.gz` - Single sample for quick tests
- [ ] Document archive contents and expected values (README in qa/archives/)
- [ ] Create archive generation scripts (for regenerating with different PCP versions)

#### 5.4.2 Test Script Infrastructure
Create `qa/` directory structure:
```
qa/
├── archives/          # Test archive files
│   ├── htop-basic.tar.gz
│   ├── README.md      # Archive documentation
│   └── generate.sh    # Archive generation scripts
├── scripts/           # Test runner scripts
│   ├── run-tests.sh   # Main test runner
│   ├── test-lib.sh    # Common test functions
│   └── archive-lib.sh # Archive handling utilities
├── tests/             # Individual test cases
│   ├── test-archive-replay.sh
│   ├── test-navigation.sh
│   ├── test-metrics.sh
│   ├── test-process-list.sh
│   └── test-ui-elements.sh
└── expected/          # Expected output files
    ├── basic-cpu.txt
    ├── basic-memory.txt
    └── process-counts.txt
```

#### 5.4.3 Core Test Scripts
- [ ] `qa/scripts/run-tests.sh` - Main test harness:
  - [ ] Discovers and runs all tests in `qa/tests/`
  - [ ] Sets up test environment (extract archives, set $PCP_ARCHIVE)
  - [ ] Captures test results and generates report
  - [ ] Exit code 0 for pass, non-zero for failures
  - [ ] TAP (Test Anything Protocol) output format

- [ ] `qa/scripts/test-lib.sh` - Common test utilities:
  - [ ] `setup_test()` - Initialize test environment
  - [ ] `cleanup_test()` - Clean up after test
  - [ ] `run_pcp_htop()` - Run pcp-htop with specific options
  - [ ] `assert_equals()` - Compare expected vs actual values
  - [ ] `assert_contains()` - Check output contains expected text
  - [ ] `extract_metric()` - Parse metric values from pcp-htop output
  - [ ] `verify_timestamp()` - Check archive timestamp display

- [ ] `qa/scripts/archive-lib.sh` - Archive-specific utilities:
  - [ ] `extract_archive()` - Unpack test archive
  - [ ] `get_archive_bounds()` - Extract start/end times using pminfo
  - [ ] `navigate_archive()` - Send navigation keystrokes to pcp-htop
  - [ ] `capture_screen()` - Capture pcp-htop screen output at specific time

#### 5.4.4 Archive Replay Test Cases

- [ ] `qa/tests/test-archive-replay.sh` - Archive replay functionality:
  - [ ] Load archive and verify initial timestamp
  - [ ] Step forward one sample, verify timestamp increment
  - [ ] Step backward one sample, verify timestamp decrement
  - [ ] Jump forward 5 minutes, verify position
  - [ ] Jump backward 5 minutes, verify position
  - [ ] Seek to archive start, verify timestamp
  - [ ] Seek to archive end, verify timestamp
  - [ ] Verify pause/unpause behavior
  - [ ] Test PM_ERR_EOL handling at boundaries

- [ ] `qa/tests/test-navigation.sh` - Navigation controls:
  - [ ] Test Left/Right arrow keys
  - [ ] Test Ctrl+Left/Right for time jumps
  - [ ] Test Home/End keys
  - [ ] Test Space for pause toggle
  - [ ] Verify function bar updates in archive mode
  - [ ] Test archive time display updates

#### 5.4.5 Metric Validation Tests

- [ ] `qa/tests/test-metrics.sh` - Metric accuracy:
  - [ ] Load known archive, verify CPU percentages match expected values
  - [ ] Verify memory metrics (total, used, free, buffers, cache)
  - [ ] Verify load averages at specific timestamps
  - [ ] Verify disk I/O counters
  - [ ] Verify network I/O counters
  - [ ] Compare pcp-htop output with `pmval` output for same archive/time
  - [ ] Test metric values don't change when paused

- [ ] `qa/tests/test-process-list.sh` - Process table validation:
  - [ ] Verify process count at known timestamps
  - [ ] Verify specific PIDs present/absent at different times
  - [ ] Verify process CPU/memory values match archive data
  - [ ] Test process sorting (by CPU, memory, PID, etc.)
  - [ ] Test process filtering in archive mode
  - [ ] Verify process start times from archive

#### 5.4.6 UI/Display Tests

- [ ] `qa/tests/test-ui-elements.sh` - Visual element validation:
  - [ ] Verify archive mode indicator appears
  - [ ] Verify timestamp display format
  - [ ] Verify paused/playing state indicator
  - [ ] Verify function bar shows archive controls
  - [ ] Test screen resizing in archive mode
  - [ ] Verify header displays archive hostname

#### 5.4.7 Automated Test Execution

- [ ] Integrate with CI/CD (GitHub Actions):
  ```yaml
  name: PCP Archive Tests
  on: [push, pull_request]
  jobs:
    test:
      runs-on: ubuntu-latest
      steps:
        - uses: actions/checkout@v3
        - name: Install PCP
          run: sudo apt-get install pcp libpcp3-dev
        - name: Build pcp-htop
          run: ./autogen.sh && ./configure --enable-pcp && make
        - name: Run archive tests
          run: ./qa/scripts/run-tests.sh
  ```

- [ ] Create test make targets:
  - [ ] `make check-archives` - Run archive replay tests
  - [ ] `make check-metrics` - Run metric validation tests
  - [ ] `make check-all` - Run all tests

### 5.5 Future Test Extensions (Follow-on Work)

#### 5.5.1 Extended htop Functionality Tests
Using the archive test framework, add tests for:
- [ ] Tree view mode with archives
- [ ] Search/filter functionality
- [ ] Column sorting and ordering
- [ ] Meter configurations
- [ ] Color schemes
- [ ] Process following mode
- [ ] Process priority/nice operations (read-only in archive mode)

#### 5.5.2 Cross-Platform Archive Tests
- [ ] Test Linux archives on Linux
- [ ] Test archives from different kernel versions
- [ ] Test archives with different metric availability
- [ ] Test archives from containers (Docker, LXC)

#### 5.5.3 Performance Benchmarks
- [ ] Measure archive load time for various sizes
- [ ] Benchmark navigation speed (forward/backward)
- [ ] Memory usage with large archives
- [ ] Screen refresh rate in archive mode
- [ ] Compare live vs archive mode performance

#### 5.5.4 Comparison Testing
- [ ] Compare pcp-htop archive replay with pcp-atop
- [ ] Compare metric values with pmchart/pmval
- [ ] Validate against native htop on live systems
- [ ] Cross-check calculations (CPU%, memory%, etc.)

#### 5.5.5 Documentation Tests
- [ ] Test all examples in man page work correctly
- [ ] Verify help screen accuracy
- [ ] Test command-line options
- [ ] Validate configuration file handling

### 5.6 Test Documentation
- [ ] Create `qa/README.md` explaining test structure
- [ ] Document how to add new tests
- [ ] Document how to create test archives
- [ ] Add troubleshooting guide for test failures
- [ ] Document expected test execution time

## Phase 6: Documentation

### 6.1 User Documentation
- [ ] Update `pcp-htop.5` man page with archive replay instructions:
  - [ ] Add ARCHIVE MODE section describing replay functionality
  - [ ] Document navigation keybindings
  - [ ] Add examples using sample archives
  - [ ] Document limitations in archive mode

- [ ] Add archive navigation keys to help screen (F1):
  - [ ] List all archive-specific keybindings
  - [ ] Explain pause/play functionality
  - [ ] Show time navigation shortcuts

- [ ] Update README with archive replay examples:
  - [ ] Basic usage: `pcp -a <archive> htop`
  - [ ] Navigation examples
  - [ ] Common use cases (debugging, performance analysis)

### 6.2 Developer Documentation
- [ ] Add comments to new code following htop conventions
- [ ] Document Platform struct archive fields
- [ ] Document pmSetMode usage patterns
- [ ] Add architecture diagram for archive replay flow

### 6.3 Testing Documentation
- [ ] Write `qa/README.md` with testing overview
- [ ] Document test archive creation process
- [ ] Provide test writing guidelines
- [ ] Document CI/CD integration

## Implementation Order

### Immediate Implementation (This Session)
1. Phase 1.1-1.2: State tracking and initialization
2. Phase 2: Metric fetching with pmSetMode
3. Phase 3.1-3.2: Basic actions and key bindings
4. Manual testing with real archives
5. Phase 3.3-3.4: UI updates
6. Phase 5.1-5.3: Basic archive replay testing

### Follow-on Implementation (Next Session)
7. Phase 4: Enhanced features (speed control, seek dialog, etc.)
8. Phase 5.4: Test infrastructure framework
9. Phase 5.5: Extended htop tests using archives
10. Phase 6: Complete documentation

### Long-term Goals
- Comprehensive test coverage for all htop features using archives
- Automated regression testing in CI/CD
- Performance benchmarking suite
- Cross-platform archive compatibility testing

## Notes

- Keep changes modular - archive code should not break live mode
- Use `opts.context == PM_CONTEXT_ARCHIVE` check to gate all archive-specific behavior
- `opts` is a global `pmOptions` struct defined in pcp/Platform.c
- `replayMode` maps to pmSetMode() API:
  - `PM_MODE_FORW` - move forward through archive
  - `PM_MODE_BACK` - move backward through archive
- When `paused` is true, re-fetch at current position without advancing
- Maintain compatibility with older PMAPI versions (check `PMAPI_VERSION`)
- Follow existing htop code style and conventions
- Test frequently with real archives during development
