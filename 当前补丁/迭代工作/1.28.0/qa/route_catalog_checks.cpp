#include "../native/route_catalog.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <string>

int main() {
    std::set<std::string> ids;
    for (const auto& route : routes::catalog()) {
        assert(route.id && *route.id && ids.insert(route.id).second);
        assert(route.zh && *route.zh && route.en && *route.en && route.period && *route.period);
        assert(route.noteZh && *route.noteZh && route.noteEn && *route.noteEn);
        assert(!route.sources.empty());
        for (const char* url : route.sources) assert(std::string(url).find("https://") == 0);
        for (const auto& stop : route.stops) {
            assert(stop.zh && *stop.zh && stop.en && *stop.en);
            assert(routes::validPoint(stop.position));
        }
        auto points = routes::pointsFor(route);
        assert(routes::validRoute(points));
        auto start = routes::rasterize(points, 1120, 560, 0, 0);
        auto middle = routes::rasterize(points, 1120, 560, (points.size() - 1) / 2.0, 0);
        auto full = routes::rasterize(points, 1120, 560, double(points.size() - 1), 0);
        assert(start.size() == 1120u * 560u && full.size() == start.size());
        assert(std::count(start.begin(), start.end(), 1) < std::count(middle.begin(), middle.end(), 1));
        assert(std::count(middle.begin(), middle.end(), 1) < std::count(full.begin(), full.end(), 1));
        for (size_t i = 0; i < full.size(); ++i) {
            assert(!start[i] || middle[i]);
            assert(!middle[i] || full[i]);
        }
        // Check every quarter-stage, not only the start, midpoint and end.
        // A previous sampler could recolor different cells as it advanced.
        auto previous = start;
        for (size_t frame = 1; frame <= (points.size() - 1) * 4; ++frame) {
            auto current = routes::rasterize(points, 1120, 560, frame / 4.0, 0);
            for (size_t i = 0; i < current.size(); ++i) assert(!previous[i] || current[i]);
            previous.swap(current);
        }
        std::cout << "PASS " << route.id << " (" << points.size() << " milestones)\n";
    }
    assert(ids.size() == 42); // Forty-two entries are present; historical review remains separate.
    std::cout << "Draft catalog geometry and metadata checks passed.\n";
}
