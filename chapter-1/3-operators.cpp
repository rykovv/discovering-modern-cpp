#include <iostream>
#include <cmath>
#include <numbers>

using std::cout;

const float pi = 3.141592653589;

int main() {
    float a = 2.0, l = 3.0, b = 4.0, h = 5.0, r = 6.0;
    // a_* - curved surface area
    // v_* - volume
    float   a_cuboid, v_cuboid, \
            a_cube, v_cube, \
            a_right_circular_cilinder, v_right_circular_cilinder, \
            a_sphere, v_sphere, \
            a_solid_hemisphere, v_solid_hemisphere;

    v_cube = a * 2;
    a_cube = v_cube * 4;

    v_cuboid = l * b * h;
    a_cuboid = 2 * (l + b) * h;

    a_right_circular_cilinder = 2 * pi * r * h;
    v_right_circular_cilinder = r * a_right_circular_cilinder / 2;

    a_sphere = 4 * pi * r * r;
    v_sphere = r * a_sphere / 3;

    a_solid_hemisphere = a_sphere / 2;
    v_solid_hemisphere = r * a_solid_hemisphere / 3;

    cout << "a = " << a << ", l = " << l << ", b = " << b << ", h = " << h << ", r = " <<  r << std::endl;
    cout << "curved surface area of cuboid = " << a_cuboid << ", volume of cuboid = " << v_cuboid << std::endl;
    cout << "curved surface area of cube = " << a_cube << ", volume of cube = " << v_cube << std::endl;
    cout << "curved surface area of right circular cilinder = " << a_right_circular_cilinder << ", volume of right circular cilinder = " << v_right_circular_cilinder << std::endl;
    cout << "curved surface area of sphere = " << a_sphere << ", volume of sphere = " << v_sphere << std::endl;
    cout << "curved surface area of solid hemisphere = " << a_solid_hemisphere << ", volume of solid hemisphere = " << v_solid_hemisphere << std::endl;

    return 0;
}