#include <cassert>

double EPS = 1e-10;

double meter2km(double m) {
    return m/1e3;
}

double usgallon2liter(double usgallon) {
    return usgallon * 3.785411784;
}

int main() {
    assert( 1.0 - meter2km(1000.0) < EPS );
    assert( 3.785411784 - usgallon2liter(1.0) < EPS);

    return 0;
}