#include <stdio.h>
#include <stdlib.h>

typedef struct { int x; int y; } Point;
typedef int Length;
typedef void (*Callback)(int, void *);

struct Node {
    int value;
    struct Node *next;
};

enum Color { RED, GREEN, BLUE };

static int compare(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}

int *allocate(int n) {
    int *p = malloc(n * sizeof(int));
    if (p == NULL) return NULL;
    for (int i = 0; i < n; i++) {
        p[i] = 0;
    }
    return p;
}

void process(Point *pt, Callback cb) {
    if (pt == NULL) return;
    while (pt->x > 0) {
        cb(pt->x, pt);
        pt->x--;
    }
    switch (pt->y) {
        case 0: break;
        default: cb(pt->y, pt); break;
    }
}

int main(int argc, char **argv) {
    Point p = {10, 20};
    int arr[] = {3, 1, 4, 1, 5};
    qsort(arr, 5, sizeof(int), compare);
    process(&p, NULL);
    return 0;
}
