## Test 01: Unix Utilities [1 pts]

Runs 'ls /'  with custom memory allocator

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

expected=$(ls /)
ls /
actual=$(LD_PRELOAD=./allocator.so ls /) || test_end
LD_PRELOAD=./allocator.so ls /
compare <(echo "${expected}") <(echo "${actual}") || test_end

echo "${expected}"
echo "${actual}"
tput cols
Expected Output                        | Actual Output
tput cols
---------------------------------------V---------------------------------------
Applications                              Applications
Library                                   Library
Network                                   Network
System                                    System
Users                                     Users
Volumes                                   Volumes
bin                                       bin
cores                                     cores
dev                                       dev
etc                                       etc
home                                      home
installer.failurerequests                 installer.failurerequests
net                                       net
private                                   private
sbin                                      sbin
tmp                                       tmp
usr                                       usr
var                                       var
tput cols
---------------------------------------^---------------------------------------

test_end
```

## Test 02: free() tests [1 pts]

Makes a large amount of random allocations and frees them

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

# If this crashes, you likely have an error in your free() / reuse logic that is
# causing memory to leak and eventually run out.
LD_PRELOAD=./allocator.so tests/progs/free
./tests/02-Free-1.sh: line 11: 14490 Segmentation fault: 11  LD_PRELOAD=./allocator.so tests/progs/free

test_end
--------------------------------------
ERROR: program terminated with SIGSEGV
--------------------------------------
```

## Test 03: Basic First Fit [1 pts]

```

output=$( \
    ALLOCATOR_ALGORITHM=first_fit \
    tests/progs/allocations-1 2> /dev/null)
     ALLOCATOR_ALGORITHM=first_fit     tests/progs/allocations-1 2> /dev/null

echo "${output}"
-- Current Memory State --

[REGION] 0x1091a7000-0x1091a8000 4096
[BLOCK]  0x1091a7000-0x1091a7098 (0) 152 152 104
[BLOCK]  0x1091a7098-0x1091a7130 (6) 152 64 16
[BLOCK]  0x1091a7130-0x1091a71c8 (2) 152 152 104
[BLOCK]  0x1091a71c8-0x1091a7208 (3) 64 0 0
[BLOCK]  0x1091a7208-0x1091a72a0 (4) 152 152 104
[BLOCK]  0x1091a72a0-0x1091a8000 (5) 3424 152 104

# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)
grep '\[BLOCK\]' <<< "${output}"     | sed 's/.*(\([0-9]*\)).*/\1/g'     | head --lines=-1
head: illegal option -- -
usage: head [-n lines | -c bytes] [file ...]

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end
echo "${expected_order}"

echo "${block_order}"
tput cols
Expected Output                        | Actual Output
tput cols
---------------------------------------V---------------------------------------
0                                      |
6                                      <
2                                      <
3                                      <
4                                      <
5                                      <
tput cols
---------------------------------------^---------------------------------------
```

## Test 04: Basic Best Fit [1 pts]

```

output=$( \
    ALLOCATOR_ALGORITHM=best_fit \
    tests/progs/allocations-1 2> /dev/null)
     ALLOCATOR_ALGORITHM=best_fit     tests/progs/allocations-1 2> /dev/null

echo "${output}"
-- Current Memory State --

[REGION] 0x10da45000-0x10da46000 4096
[BLOCK]  0x10da45000-0x10da45098 (0) 152 152 104
[BLOCK]  0x10da45098-0x10da45130 (1) 152 0 0
[BLOCK]  0x10da45130-0x10da451c8 (2) 152 152 104
[BLOCK]  0x10da451c8-0x10da45208 (6) 64 64 16
[BLOCK]  0x10da45208-0x10da452a0 (4) 152 152 104
[BLOCK]  0x10da452a0-0x10da46000 (5) 3424 152 104

# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)
grep '\[BLOCK\]' <<< "${output}"     | sed 's/.*(\([0-9]*\)).*/\1/g'     | head --lines=-1
head: illegal option -- -
usage: head [-n lines | -c bytes] [file ...]

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end
echo "${expected_order}"

echo "${block_order}"
tput cols
Expected Output                        | Actual Output
tput cols
---------------------------------------V---------------------------------------
0                                      |
1                                      <
2                                      <
6                                      <
4                                      <
5                                      <
tput cols
---------------------------------------^---------------------------------------
```

## Test 05: Basic Worst Fit [1 pts]

```

output=$( \
    ALLOCATOR_ALGORITHM=worst_fit \
    tests/progs/allocations-1 2> /dev/null)
     ALLOCATOR_ALGORITHM=worst_fit     tests/progs/allocations-1 2> /dev/null

echo "${output}"
-- Current Memory State --

[REGION] 0x1089a8000-0x1089a9000 4096
[BLOCK]  0x1089a8000-0x1089a8098 (0) 152 152 104
[BLOCK]  0x1089a8098-0x1089a8130 (1) 152 0 0
[BLOCK]  0x1089a8130-0x1089a81c8 (2) 152 152 104
[BLOCK]  0x1089a81c8-0x1089a8208 (3) 64 0 0
[BLOCK]  0x1089a8208-0x1089a82a0 (4) 152 152 104
[BLOCK]  0x1089a82a0-0x1089a8338 (5) 152 152 104
[BLOCK]  0x1089a8338-0x1089a9000 (6) 3272 64 16

# Just get the block ordering from the output. We ignore the last allocation
# that is caused by printing to stdout.
block_order=$(grep '\[BLOCK\]' <<< "${output}" \
    | sed 's/.*(\([0-9]*\)).*/\1/g' \
    | head --lines=-1)
grep '\[BLOCK\]' <<< "${output}"     | sed 's/.*(\([0-9]*\)).*/\1/g'     | head --lines=-1
head: illegal option -- -
usage: head [-n lines | -c bytes] [file ...]

compare <(echo "${expected_order}") <(echo "${block_order}") || test_end

echo "${expected_order}"
echo "${block_order}"
tput cols
Expected Output                        | Actual Output
tput cols
---------------------------------------V---------------------------------------
0                                      |
1                                      <
2                                      <
3                                      <
4                                      <
5                                      <
6                                      <
tput cols
---------------------------------------^---------------------------------------
```

## Test 06: Testing first_fit [1 pts]

```

# Check to make sure there were no extra pages
ALLOCATOR_ALGORITHM=${algo} \
tests/progs/allocations-2 2> /dev/null || test_end
./tests/06-First-Fit-1.sh: line 20: 14551 Segmentation fault: 11  ALLOCATOR_ALGORITHM=${algo} tests/progs/allocations-2 2> /dev/null
--------------------------------------
ERROR: program terminated with SIGSEGV
--------------------------------------
```

## Test 07: Testing best_fit [1 pts]

```

# Check to make sure there were no extra pages
ALLOCATOR_ALGORITHM=${algo} \
tests/progs/allocations-2 2> /dev/null || test_end
./tests/07-Best-Fit-1.sh: line 19: 14558 Segmentation fault: 11  ALLOCATOR_ALGORITHM=${algo} tests/progs/allocations-2 2> /dev/null
--------------------------------------
ERROR: program terminated with SIGSEGV
--------------------------------------
```

## Test 08: Testing worst_fit [1 pts]

```

# Check to make sure there were no extra pages
ALLOCATOR_ALGORITHM=${algo} \
tests/progs/allocations-2 2> /dev/null || test_end
./tests/08-Worst-Fit-1.sh: line 20: 14565 Segmentation fault: 11  ALLOCATOR_ALGORITHM=${algo} tests/progs/allocations-2 2> /dev/null
--------------------------------------
ERROR: program terminated with SIGSEGV
--------------------------------------
```

## Test 09: Memory Scribbling [1 pts]

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

actual=$(LD_PRELOAD=./allocator.so tests/progs/scribble) || test_end
LD_PRELOAD=./allocator.so tests/progs/scribble

compare <(echo "${expected}") <(echo "${actual}")

echo "${expected}"
echo "${actual}"
tput cols
Expected Output                        | Actual Output
tput cols
---------------------------------------V---------------------------------------
Printing uninitialized variables:         Printing uninitialized variables:
-1431655766                            |  0
-1431655766                            |  42
-1431655766                            |  0
12297829382473034410                   |  2305843009213693952
aaaaaaaa                               |  0
aaaaaaaaaaaaaaaa                       |  2000000000000000
Totalling up uninitialized arrays:        Totalling up uninitialized arrays:
850000                                 |  0
calloc should still zero out the memor    calloc should still zero out the memor
0                                         0
tput cols
---------------------------------------^---------------------------------------

test_end
```

## Test 10: Thread Safety [1 pts]

Performs allocations across multiple threads in parallel

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

# If this crashes or times out, your allocator is not thread safe!
LD_PRELOAD=./allocator.so tests/progs/thread-safety
Performed 25000 multi-threaded allocations in 1.71s

test_end
```

## Test 11: print_memory() function [1 pts]

```

output=$(tests/progs/print-test 2> /dev/null)
tests/progs/print-test 2> /dev/null

echo "${output}"
-- Current Memory State --

[REGION] 0x10528f000-0x105290000 4096
[BLOCK]  0x10528f000-0x105290000 (0) 4096 4048 4000
[REGION] 0x105290000-0x105291000 4096
[BLOCK]  0x105290000-0x105291000 (1) 4096 4048 4000
[REGION] 0x105291000-0x105292000 4096
[BLOCK]  0x105291000-0x105292000 (2) 4096 4048 4000

# Must have 3 or more regions
regions=$(grep '\[REGION\]' <<< "${output}" | wc -l)
grep '\[REGION\]' <<< "${output}" | wc -l
[[ ${regions} -ge 3 ]] || test_end 1

# Must have 3 or more allocations of req size 4000
# We also check that the request size is in column 6
blocks=$(grep '\[BLOCK\]' <<< "${output}" \
    | awk '{print $6}' \
    | grep '4000' \
    | wc -l)
grep '\[BLOCK\]' <<< "${output}"     | awk '{print $6}'     | grep '4000'     | wc -l
[[ ${blocks} -ge 3 ]] || test_end 1

test_end
```

## Test 12: write_memory() function [1 pts]

```

rm -f test-memory-output-file.txt

tests/progs/write-test test-memory-output-file.txt 2> /dev/null

output=$(cat test-memory-output-file.txt)
cat test-memory-output-file.txt
cat: test-memory-output-file.txt: No such file or directory

# Must have 3 or more regions
regions=$(grep '\[REGION\]' <<< "${output}" | wc -l)
grep '\[REGION\]' <<< "${output}" | wc -l
[[ ${regions} -ge 3 ]] || test_end 1
```

## Test 13: You get a point just for TRYING! [1 pts]

```

test_end 0
```

## Test 14: Unix Utilities [1 pts]

Runs 'df' and 'w' with custom memory allocator

```

# Check to make sure the library exists
[[ -e "./allocator.so" ]] || test_end 1

LD_PRELOAD=./allocator.so   df  || test_end
Filesystem    512-blocks      Used Available Capacity iused               ifree %iused  Mounted on
/dev/disk1s1   236568496 212879144  16050680    93% 1314984 9223372036853460823    0%   /
devfs                385       385         0   100%     666                   0  100%   /dev
/dev/disk1s4   236568496   6291736  16050680    29%       3 9223372036854775804    0%   /private/var/vm
map -hosts             0         0         0   100%       0                   0  100%   /net
map auto_home          0         0         0   100%       0                   0  100%   /home
LD_PRELOAD=./allocator.so   w   || test_end
14:43  up 8 days, 15:16, 2 users, load averages: 1.32 1.05 1.26
USER     TTY      FROM              LOGIN@  IDLE WHAT
elijahdelosreyes console  -                20Apr19 8days -
elijahdelosreyes s000     -                14:37       - tmux

test_end
```

