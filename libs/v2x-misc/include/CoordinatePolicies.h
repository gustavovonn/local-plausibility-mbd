#ifndef COORDINATE_POLICIES_H_
#define COORDINATE_POLICIES_H_

#include "Constants.h"
#include <deque>
#include <cmath>
#include <algorithm>

// -----------------------------------------------------------------------------
// Cartesian Coordinate Policy (Euclidean 2D metric space, e.g. SUMO / VeReMi)
// -----------------------------------------------------------------------------
struct CartesianCoordinatePolicy {
    static constexpr const char* name() { return "Cartesian"; }

    /**
     * @brief Apply a local metric displacement to a position.
     *
     * Euclidean space: metres map onto coordinates one-to-one.
     * In KinematicState / PredictedState: lon stores X [m], lat stores Y [m].
     */
    static void translate(double lat, double lon, double dx, double dy,
                          double& outLat, double& outLon)
    {
        outLat = lat + dy;
        outLon = lon + dx;
    }

    /**
     * @brief Euclidean distance between two points in meters.
     */
    static double distance(double pos1_a, double pos2_a,
                           double pos1_b, double pos2_b)
    {
        return std::hypot(pos1_b - pos1_a, pos2_b - pos2_a);
    }

    /**
     * @brief Inter-frame spatial displacement in meters.
     */
    static double displacement(double pos1_a, double pos2_a,
                               double pos1_b, double pos2_b)
    {
        return distance(pos1_a, pos2_a, pos1_b, pos2_b);
    }

    // Bearing from a to b, clockwise from +Y [rad]; lat stores Y, lon stores X
    static double bearing(double lat_a, double lon_a, double lat_b, double lon_b)
    {
        return std::atan2(lon_b - lon_a, lat_b - lat_a);
    }
};

// -----------------------------------------------------------------------------
// Geographic Coordinate Policy (WGS-84 Ellipsoid, e.g. GPS / Artery CAM)
// -----------------------------------------------------------------------------
struct Wgs84CoordinatePolicy {
    static constexpr const char* name() { return "WGS84"; }

    /**
     * @brief Apply a local metric displacement to a geodetic position.
     *
     * Projects an east/north displacement onto the WGS-84 ellipsoid using the
     * radii of curvature evaluated at the starting latitude.
     */
    static void translate(double lat, double lon, double dx, double dy,
                          double& outLat, double& outLon)
    {
        const double latRad = lat * ConstantsWGS84::DEG_TO_RAD;
        const double R_phi = ConstantsWGS84::meridionalRadius(latRad);       // N-S [m]
        const double R_lambda = ConstantsWGS84::primeVerticalRadius(latRad); // E-W [m]
        const double cosLat = std::cos(latRad);

        outLat = lat + (dy / R_phi) * ConstantsWGS84::RAD_TO_DEG;
        outLon = lon + (dx / (R_lambda * cosLat)) * ConstantsWGS84::RAD_TO_DEG;
    }

    /**
     * @brief Ellipsoidal equirectangular distance between two (lat, lon) points in meters.
     */
    static double distance(double lat1, double lon1,
                           double lat2, double lon2)
    {
        const double latRad1 = lat1 * ConstantsWGS84::DEG_TO_RAD;
        const double latRad2 = lat2 * ConstantsWGS84::DEG_TO_RAD;
        const double lonRad1 = lon1 * ConstantsWGS84::DEG_TO_RAD;
        const double lonRad2 = lon2 * ConstantsWGS84::DEG_TO_RAD;

        const double latMid = (latRad1 + latRad2) * 0.5;

        const double R_phi = ConstantsWGS84::meridionalRadius(latMid);
        const double R_lambda = ConstantsWGS84::primeVerticalRadius(latMid);

        const double dy = R_phi * (latRad2 - latRad1);
        const double dx = R_lambda * (lonRad2 - lonRad1) * std::cos(latMid);
        return std::sqrt(dx * dx + dy * dy);
    }

    /**
     * @brief Inter-frame spatial displacement in meters.
     */
    static double displacement(double lat1, double lon1,
                               double lat2, double lon2)
    {
        return distance(lat1, lon1, lat2, lon2);
    }

    // Bearing from point 1 to point 2, clockwise from true north [rad]; same local radii as distance()
    static double bearing(double lat1, double lon1, double lat2, double lon2)
    {
        const double latRad1 = lat1 * ConstantsWGS84::DEG_TO_RAD;
        const double latRad2 = lat2 * ConstantsWGS84::DEG_TO_RAD;
        const double latMid = (latRad1 + latRad2) * 0.5;

        const double dy = ConstantsWGS84::meridionalRadius(latMid) * (latRad2 - latRad1);
        const double dx = ConstantsWGS84::primeVerticalRadius(latMid) * (lon2 - lon1) * ConstantsWGS84::DEG_TO_RAD
                        * std::cos(latMid);
        return std::atan2(dx, dy);
    }
};

#endif // COORDINATE_POLICIES_H_
