#include "../native/route_geometry.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

int main() {
    using routes::Point;
    using routes::rasterize;
    const int w = 360, h = 180;
    const std::vector<Point> route = {{0, 0}, {0, 10}, {10, 10}};
    auto start = rasterize(route, w, h, 0);
    auto half = rasterize(route, w, h, 0.5);
    auto first = rasterize(route, w, h, 1);
    auto full = rasterize(route, w, h, 2);
    assert(start.size() == size_t(w * h));
    auto count = [](const auto& v) { return std::count(v.begin(), v.end(), 1); };
    assert(count(start) == 1 && count(start) < count(half));
    assert(count(half) < count(first) && count(first) < count(full));
    for (size_t i = 0; i < full.size(); ++i) {
        assert(!start[i] || half[i]);
        assert(!half[i] || first[i]);
        assert(!first[i] || full[i]);
    }
    auto wrap = rasterize({{0, 179}, {0, -179}}, w, h, 1);
    assert(wrap[90 * w + 359] && wrap[90 * w] && wrap[90 * w + 1]);
    assert(!wrap[90 * w + 180]);
    auto gap = rasterize({{0, 0}, {0, 20, true}}, w, h, 1);
    assert(gap[90 * w + 180] && gap[90 * w + 200]);
    assert(!gap[90 * w + 190]);
    auto pole = rasterize({{90, -180}, {-90, -180}}, w, h, 1);
    assert(pole[0] && pole[(h - 1) * w]);
    assert(rasterize({{91, 0}, {0, 0}}, w, h, 1).empty());
    assert(rasterize(route, w, h, std::numeric_limits<double>::quiet_NaN()).empty());
    assert(rasterize(route, 0, h, 1).empty());
    std::cout << "Route geometry: progress, dateline, gaps, poles and invalid inputs passed.\n";
}
