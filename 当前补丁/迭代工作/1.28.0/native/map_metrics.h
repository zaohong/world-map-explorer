#pragma once
#include <algorithm>
#include <cmath>

struct MapScale { double kilometers = 0; int pixels = 0; double latitude = 0; };

// WGS 84 parallel radius, sampled at the latitude of the displayed scale bar.
// An equirectangular map has no single scale valid at every latitude.
static MapScale mapScaleForView(int mapWidth, double zoom, double latitude, int maxPixels) {
    MapScale result;
    if (mapWidth <= 0 || maxPixels < 1 || !std::isfinite(zoom) || zoom <= 0 || !std::isfinite(latitude) || std::fabs(latitude) >= 89.9) return result;
    constexpr double pi = 3.14159265358979323846, a = 6378.137, e2 = 0.0066943799901413165;
    double phi = latitude*pi/180.0, s = std::sin(phi);
    double radius = a*std::cos(phi)/std::sqrt(1.0-e2*s*s);
    double kmPerPixel = 2.0*pi*radius/(mapWidth*zoom), target = maxPixels*kmPerPixel;
    if (!(target > 0) || !std::isfinite(target)) return result;
    double unit = std::pow(10.0,std::floor(std::log10(target))), fraction = target/unit;
    result.kilometers = (fraction >= 5 ? 5 : fraction >= 2 ? 2 : 1)*unit;
    result.pixels = std::max(1,(int)std::lround(result.kilometers/kmPerPixel));
    result.latitude = latitude;
    return result;
}
