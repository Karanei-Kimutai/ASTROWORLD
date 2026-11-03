/**
 * @file clipping.cpp
 * @brief Cohen-Sutherland line clipping implementation.
 */
#include "raster/clipping.h"

int computeOutCode(const Vec2& point, const ClipWindow& window) {
    int code = outCodeInside;
    if (point.x < window.xMin) code |= outCodeLeft;
    else if (point.x > window.xMax) code |= outCodeRight;
    if (point.y < window.yMin) code |= outCodeAbove;
    else if (point.y > window.yMax) code |= outCodeBelow;
    return code;
}

bool clipLineCohenSutherland(Vec2& start, Vec2& end, const ClipWindow& window) {
    int startCode = computeOutCode(start, window);
    int endCode = computeOutCode(end, window);

    while (true) {
        if ((startCode | endCode) == 0) return true;   // trivial accept
        if ((startCode & endCode) != 0) return false;  // trivial reject

        // At least one endpoint is outside: choose it.
        const int outsideCode = startCode != 0 ? startCode : endCode;
        const double deltaX = end.x - start.x;
        const double deltaY = end.y - start.y;
        Vec2 intersection;

        // Intersect with the violated edge using the parametric line equation.
        if (outsideCode & outCodeAbove) {
            intersection.x = start.x + deltaX * (window.yMin - start.y) / deltaY;
            intersection.y = window.yMin;
        } else if (outsideCode & outCodeBelow) {
            intersection.x = start.x + deltaX * (window.yMax - start.y) / deltaY;
            intersection.y = window.yMax;
        } else if (outsideCode & outCodeRight) {
            intersection.y = start.y + deltaY * (window.xMax - start.x) / deltaX;
            intersection.x = window.xMax;
        } else {  // outCodeLeft
            intersection.y = start.y + deltaY * (window.xMin - start.x) / deltaX;
            intersection.x = window.xMin;
        }

        // Replace the outside endpoint and re-classify it.
        if (outsideCode == startCode) {
            start = intersection;
            startCode = computeOutCode(start, window);
        } else {
            end = intersection;
            endCode = computeOutCode(end, window);
        }
    }
}
