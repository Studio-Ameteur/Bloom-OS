#ifndef RING_H
#define RING_H

#define RING_SEGMENTS 4
#define RING_INNER_RADIUS 110
#define RING_OUTER_RADIUS 145

static const long SegStartX[RING_SEGMENTS] = {
4362,99905,-4362,-99905
};

static const long SegStartY[RING_SEGMENTS] = {
-99905,4362,99905,-4362
};

static const long SegEndX[RING_SEGMENTS] = {
99905,4362,-99905,-4362
};

static const long SegEndY[RING_SEGMENTS] = {
-4362,99905,4362,-99905
};

#endif
