# Project 3: Memory Allocator

See: https://www.cs.usfca.edu/~mmalensek/cs326/assignments/project-3.html 

## Super Swag Team Alpha's Memory Allocator

1. What is our program?

This super sick program will replace your lame implementation of free and malloc with ours. Our implementation will flex super hard and is quite efficient if I do say so myself. Hey, it'll even reuse and free space previously allocated by other regions. That's amazing, is it not? Oh, but hold on to your panties because this baby also has thread safety AND bluetooth functionality.

2. How does it work?

Since this is propriety software, we can't disclose this, but we're feeling generous today. Our implementation uses a linked list that keeps track of current region and block allocations. In particular, these allocations contain not only memory another process is using but also our struct that holds valuable information. Simple? No, it isn't. This program primarily uses mmap and munmap, which are system calls. We WERE thinking of using sbrk to flex even harder but we didn't want Edmund to sit in the labs with us for another 51 hours.

3. How do you build it?

Fool! We have a Makefile!

```bash
make
```

It's that easy...

4. Are there any other relevant details?

You have to listen to Float On by Modest Mouse while reviewing the source code because that song is really lit and provided a lot of inspiration and determination into this mediocre project.

## Compiling

To compile and use the allocator:

```bash
make
LD_PRELOAD=$(pwd)/allocator.so ls /
```

(in this example, the command `ls /` is run with the custom memory allocator instead of the default).

## Testing

To execute the test cases, use `make test`. To pull in updated test cases, run `make testupdate`. You can also run a specific test case instead of all of them:

```
# Run all test cases:
make test

# Run a specific test case:
make test run=4

# Run a few specific test cases (4, 8, and 12 in this case):
make test run='4 8 12'
```

## Visualization

Memory State when we run Robert's Project 2:

```bash
-- Current Memory State --
[REGION] 0x7f89b92ab000-0x7f89b92ac000 4096
[BLOCK]  0x7f89b92ab000-0x7f89b92ab258 (0) 600 600 552
[BLOCK]  0x7f89b92ab258-0x7f89b92ab298 (1) 64 64 16
[BLOCK]  0x7f89b92ab298-0x7f89b92ab340 (2) 168 0 0
[BLOCK]  0x7f89b92ab340-0x7f89b92ab388 (4) 72 72 24
[BLOCK]  0x7f89b92ab388-0x7f89b92ab3f0 (5) 104 104 56
[BLOCK]  0x7f89b92ab3f0-0x7f89b92ab460 (6) 112 112 64
[BLOCK]  0x7f89b92ab460-0x7f89b92ab4c8 (7) 104 104 56
[BLOCK]  0x7f89b92ab4c8-0x7f89b92ab510 (8) 72 72 24
[BLOCK]  0x7f89b92ab510-0x7f89b92ab578 (9) 104 104 56
[BLOCK]  0x7f89b92ab578-0x7f89b92ab5e8 (10) 112 112 64
[BLOCK]  0x7f89b92ab5e8-0x7f89b92ab650 (11) 104 104 56
[BLOCK]  0x7f89b92ab650-0x7f89b92ab698 (12) 72 72 24
[BLOCK]  0x7f89b92ab698-0x7f89b92ab700 (13) 104 104 56
[BLOCK]  0x7f89b92ab700-0x7f89b92ab750 (14) 80 80 32
[BLOCK]  0x7f89b92ab750-0x7f89b92ab7b8 (15) 104 104 56
[BLOCK]  0x7f89b92ab7b8-0x7f89b92ab800 (16) 72 72 24
[BLOCK]  0x7f89b92ab800-0x7f89b92ab868 (17) 104 104 56
[BLOCK]  0x7f89b92ab868-0x7f89b92ab8d8 (18) 112 112 64
[BLOCK]  0x7f89b92ab8d8-0x7f89b92ab948 (19) 112 112 64
[BLOCK]  0x7f89b92ab948-0x7f89b92ab9b0 (20) 104 104 56
[BLOCK]  0x7f89b92ab9b0-0x7f89b92aba18 (21) 104 104 56
[BLOCK]  0x7f89b92aba18-0x7f89b92aba68 (22) 80 80 32
[BLOCK]  0x7f89b92aba68-0x7f89b92abad0 (23) 104 104 56
[BLOCK]  0x7f89b92abad0-0x7f89b92abb20 (24) 80 80 32
[BLOCK]  0x7f89b92abb20-0x7f89b92abb88 (25) 104 104 56
[BLOCK]  0x7f89b92abb88-0x7f89b92abbd8 (26) 80 80 32
[BLOCK]  0x7f89b92abbd8-0x7f89b92abc40 (27) 104 104 56
[BLOCK]  0x7f89b92abc40-0x7f89b92abc88 (28) 72 72 24
[BLOCK]  0x7f89b92abc88-0x7f89b92abcf0 (29) 104 104 56
[BLOCK]  0x7f89b92abcf0-0x7f89b92abd38 (30) 72 72 24
[BLOCK]  0x7f89b92abd38-0x7f89b92abda0 (31) 104 104 56
[BLOCK]  0x7f89b92abda0-0x7f89b92abdf0 (32) 80 80 32
[BLOCK]  0x7f89b92abdf0-0x7f89b92ac000 (33) 528 104 56
[REGION] 0x7f89b92a9000-0x7f89b92ab000 8192
[BLOCK]  0x7f89b92a9000-0x7f89b92aa030 (3) 4144 4144 4096
[BLOCK]  0x7f89b92aa030-0x7f89b92ab000 (34) 4048 1072 1024
```

Memory State when we run cat on a random text file
```
-- Current Memory State --
[REGION] 0x7f2016dda000-0x7f2016ddb000 4096
[BLOCK]  0x7f2016dda000-0x7f2016dda430 (1) 1072 1072 1024
[BLOCK]  0x7f2016dda430-0x7f2016dda4d8 (2) 168 168 120
[BLOCK]  0x7f2016dda4d8-0x7f2016dda518 (3) 64 64 16
[BLOCK]  0x7f2016dda518-0x7f2016dda850 (4) 824 824 776
[BLOCK]  0x7f2016dda850-0x7f2016dda8f0 (5) 160 160 112
[BLOCK]  0x7f2016dda8f0-0x7f2016ddae58 (6) 1384 1384 1336
[BLOCK]  0x7f2016ddae58-0x7f2016ddaf60 (7) 264 264 216
[BLOCK]  0x7f2016ddaf60-0x7f2016ddb000 (9) 160 152 104
[REGION] 0x7f2016dd9000-0x7f2016dda000 4096
[BLOCK]  0x7f2016dd9000-0x7f2016dd91e0 (8) 480 480 432
[BLOCK]  0x7f2016dd91e0-0x7f2016dd9268 (10) 136 136 88
[BLOCK]  0x7f2016dd9268-0x7f2016dd9310 (11) 168 168 120
[BLOCK]  0x7f2016dd9310-0x7f2016dd93e8 (12) 216 216 168
[BLOCK]  0x7f2016dd93e8-0x7f2016dd9480 (13) 152 152 104
[BLOCK]  0x7f2016dd9480-0x7f2016dd9500 (14) 128 128 80
[BLOCK]  0x7f2016dd9500-0x7f2016dd95f0 (15) 240 240 192
[BLOCK]  0x7f2016dd95f0-0x7f2016dd9630 (16) 64 64 16
[BLOCK]  0x7f2016dd9630-0x7f2016dd9670 (17) 64 64 16
[BLOCK]  0x7f2016dd9670-0x7f2016dd96b0 (18) 64 64 16
[BLOCK]  0x7f2016dd96b0-0x7f2016dd96f0 (19) 64 64 16
[BLOCK]  0x7f2016dd96f0-0x7f2016dd9730 (20) 64 64 16
[BLOCK]  0x7f2016dd9730-0x7f2016dd9770 (21) 64 64 16
[BLOCK]  0x7f2016dd9770-0x7f2016dd97b0 (22) 64 64 16
[BLOCK]  0x7f2016dd97b0-0x7f2016dd97f0 (23) 64 64 16
[BLOCK]  0x7f2016dd97f0-0x7f2016dd9830 (24) 64 64 16
[BLOCK]  0x7f2016dd9830-0x7f2016dd9870 (25) 64 64 16
[BLOCK]  0x7f2016dd9870-0x7f2016dd98b0 (26) 64 64 16
[BLOCK]  0x7f2016dd98b0-0x7f2016dd98f0 (27) 64 64 16
[BLOCK]  0x7f2016dd98f0-0x7f2016dd9930 (28) 64 64 16
[BLOCK]  0x7f2016dd9930-0x7f2016dd9988 (29) 88 88 40
[BLOCK]  0x7f2016dd9988-0x7f2016dda000 (30) 1656 64 16
```