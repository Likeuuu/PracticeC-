#include <assert.h>
#include "Arena.h"

int main()
{
    Arena arena(10240, 64);

    int*    a = static_cast<int*>(arena.allocate(sizeof(int), alignof(int)));
    double* b = static_cast<double*>(arena.allocate(sizeof(double), alignof(double)));

    assert(reinterpret_cast<uintptr_t>(a) % alignof(int) == 0);
    assert(reinterpret_cast<uintptr_t>(b) % alignof(double) == 0);

    arena.reset();

    int* c = static_cast<int*>(arena.allocate(sizeof(int), alignof(int)));
    assert(c == a);
    return 0;
}
