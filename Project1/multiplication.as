    lw  0   2   mcand
    lw  0   3   mplier
    lw  0   4   one
LOOP    nor 3   0   3
    nor 4   0   4
    nor 3   4   5
    nor 3   0   3
    nor 4   0   4
    beq 5   0   SHFT
    add 1   2   1
    beq 0   0   SHFT
SHFT    add 5   6   6
    add 2   2   2
    add 4   4   4
    beq 3   6   DONE
    beq 0   0   LOOP
    noop
DONE    halt
mcand   .fill   32766
mplier  .fill   10838
one    .fill   1
