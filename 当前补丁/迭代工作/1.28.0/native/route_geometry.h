#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// World-grid coordinates are equirectangular. A route is an approximation
// between documented waypoints, not a claim of an exact historical track.
namespace routes {
struct Point {
    double latitude;
    double longitude;
    bool breakBefore = false;
};

inline bool validPoint(const Point& p) {
    return std::isfinite(p.latitude) && std::isfinite(p.longitude) &&
           p.latitude >= -90.0 && p.latitude <= 90.0 &&
           p.longitude >= -180.0 && p.longitude <= 180.0;
}

inline bool validRoute(const std::vector<Point>& points) {
    if (points.size() < 2 || points.front().breakBefore) return false;
    for (const auto& p : points) if (!validPoint(p)) return false;
    return true;
}

inline double distanceKm(const Point& a,const Point& b){
    constexpr double rad=3.14159265358979323846/180.0;
    double dlat=(b.latitude-a.latitude)*rad,dlon=(b.longitude-a.longitude)*rad;
    double h=std::sin(dlat/2)*std::sin(dlat/2)+std::cos(a.latitude*rad)*std::cos(b.latitude*rad)*std::sin(dlon/2)*std::sin(dlon/2);
    return 12742.0176*std::asin(std::sqrt(std::max(0.0,std::min(1.0,h))));
}
// Distances measure connections between reference points, never road or sailed mileage.
// Broken segments have no connecting length and are crossed as a discrete jump.
struct DistanceProfile {
    std::vector<double> cumulative;
    DistanceProfile()=default;
    explicit DistanceProfile(const std::vector<Point>& points){
        if(!validRoute(points))return;cumulative.resize(points.size(),0);
        for(size_t i=1;i<points.size();++i)cumulative[i]=cumulative[i-1]+(points[i].breakBefore?0:distanceKm(points[i-1],points[i]));
    }
    double length()const{return cumulative.empty()?0:cumulative.back();}
    double at(double progress)const{
        if(cumulative.empty()||!std::isfinite(progress))return 0;
        progress=std::max(0.0,std::min(progress,double(cumulative.size()-1)));size_t index=(size_t)std::floor(progress);
        return index+1<cumulative.size()?cumulative[index]+(cumulative[index+1]-cumulative[index])*(progress-index):length();
    }
    double progress(double distance)const{
        if(cumulative.empty()||!std::isfinite(distance)||distance<0)return 0;
        if(distance>=length())return (double)cumulative.size()-1;
        auto upper=std::upper_bound(cumulative.begin(),cumulative.end(),distance);size_t index=(size_t)(upper-cumulative.begin());if(index==0)return 0;
        double span=cumulative[index]-cumulative[index-1];return (double)index-1+(span>0?(distance-cumulative[index-1])/span:0);
    }
    double advance(double current,double seconds,double duration)const{
        if(cumulative.empty()||!std::isfinite(current)||!std::isfinite(seconds)||!std::isfinite(duration)||seconds<0||duration<=0)return current;
        return progress(std::min(length(),at(current)+length()*seconds/duration));
    }
};

// progress is measured in waypoint-to-waypoint stages: 0 is the start,
// 1 is the second waypoint, and n-1 is the complete route.
inline std::vector<unsigned char> rasterize(const std::vector<Point>& points,
                                             int columns, int rows,
                                             double progress, int radius = 0) {
    if (!validRoute(points) || columns <= 0 || rows <= 0 ||
        columns > 10000 || rows > 10000 || !std::isfinite(progress) ||
        radius < 0 || radius > 16) return {};
    std::vector<unsigned char> cells(static_cast<size_t>(columns) * rows, 0);
    progress = std::max(0.0, std::min(progress, double(points.size() - 1)));
    auto xy = [=](const Point& p, double& x, double& y) {
        x = (p.longitude + 180.0) / 360.0 * columns;
        y = (90.0 - p.latitude) / 180.0 * rows;
        y = std::max(0.0, std::min(y, double(rows) - 1e-9));
    };
    auto mark = [&](double x, double y) {
        int cx = static_cast<int>(std::floor(x));
        int cy = static_cast<int>(std::floor(y));
        for (int dy = -radius; dy <= radius; ++dy) {
            int row = cy + dy;
            if (row < 0 || row >= rows) continue;
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx * dx + dy * dy > radius * radius) continue;
                int col = (cx + dx) % columns;
                if (col < 0) col += columns;
                cells[static_cast<size_t>(row) * columns + col] = 1;
            }
        }
    };
    double x0, y0;
    xy(points.front(), x0, y0);
    mark(x0, y0);
    for (size_t i = 1; i < points.size(); ++i) {
        double amount = std::max(0.0, std::min(1.0, progress - double(i - 1)));
        if (amount <= 0.0) break;
        double x1, y1;
        xy(points[i], x1, y1);
        if (points[i].breakBefore) {
            if (amount >= 1.0) mark(x1, y1);
        } else {
            double dx = x1 - x0;
            if (dx > columns / 2.0) dx -= columns;
            if (dx < -columns / 2.0) dx += columns;
            double dy = y1 - y0;
            // Sample positions must not move as playback advances: otherwise
            // cells that were colored in an earlier frame can disappear.
            int samples = std::max(1, static_cast<int>(std::ceil(
                std::max(std::fabs(dx), std::fabs(dy)) * 2.0)));
            int completed = amount >= 1.0 ? samples :
                static_cast<int>(std::floor(amount * samples));
            for (int n = 1; n <= completed; ++n) {
                double t = static_cast<double>(n) / samples;
                mark(x0 + dx * t, y0 + dy * t);
            }
        }
        x0 = x1; y0 = y1;
        if (amount < 1.0) break;
    }
    return cells;
}
}
