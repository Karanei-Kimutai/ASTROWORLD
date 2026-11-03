/**
 * @file astroWordmark.cpp
 * @brief Letter construction and anti-aliased rendering of the ASTROWORLD logo.
 */
#include "text/astroWordmark.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/coverageMask.h"
#include "core/simpleRandom.h"
#include "curves/bezier.h"
#include "curves/spline.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/strokes.h"

namespace {

using Contour = std::vector<Vec2>;

/** @brief How a letter is rendered. */
enum class LetterKind { maskedShape, earthGlobe };

/** @brief One letter of the logo: its character, colour and how wide it is (in letter heights). */
struct LogoLetter {
    char character;
    Color color;
    double advance;
    LetterKind kind;
};

/** @brief "ASTROWORLD" with each letter's colour, matching the reference logo. */
const std::vector<LogoLetter>& logoLetters() {
    static const std::vector<LogoLetter> letters = {
        {'A', astroPalette::letterYellow, 0.78, LetterKind::maskedShape},
        {'S', astroPalette::letterRed, 0.66, LetterKind::maskedShape},
        {'T', astroPalette::letterWhite, 0.74, LetterKind::maskedShape},
        {'R', astroPalette::letterBlue, 0.72, LetterKind::maskedShape},
        {'O', astroPalette::earthOcean, 0.86, LetterKind::earthGlobe},
        {'W', astroPalette::letterMagenta, 1.02, LetterKind::maskedShape},
        {'@', astroPalette::letterGreen, 0.82, LetterKind::maskedShape},  // '@' = the ring O
        {'R', astroPalette::letterOrange, 0.72, LetterKind::maskedShape},
        {'L', astroPalette::letterRose, 0.62, LetterKind::maskedShape},
        {'D', astroPalette::letterGrey, 0.76, LetterKind::maskedShape},
    };
    return letters;
}

// ---------------------------------------------------------------------------
// Shape builders (all in unit space: y = 0 cap line, y = 1 baseline)
// ---------------------------------------------------------------------------

/** @brief Append the points of a sampled curve to a contour. */
void appendPoints(Contour& contour, const Contour& points) { contour.insert(contour.end(), points.begin(), points.end()); }

/**
 * @brief The signature stroke of the font: wide at both ends, pinched in the middle.
 *
 * The half-width along the stroke is
 *   w(t) = waist + (endWidth(t) - waist) * |2t - 1|^2.2
 * which is 'waist' at the middle (t = 0.5) and flares quickly toward each end.
 * The ends are cut square to the stroke direction, giving flat feet and tops.
 */
Contour makeFlaredStroke(const Vec2& start, const Vec2& end, double startHalfWidth, double waistHalfWidth,
                         double endHalfWidth) {
    const Vec2 side = (end - start).normalized().perpendicular();
    const int sampleCount = 24;
    Contour leftEdge;
    Contour rightEdge;
    for (int sample = 0; sample <= sampleCount; ++sample) {
        const double t = static_cast<double>(sample) / sampleCount;
        const double endWidth = startHalfWidth + (endHalfWidth - startHalfWidth) * t;
        const double flare = std::pow(std::fabs(2.0 * t - 1.0), 2.2);
        const double halfWidth = waistHalfWidth + (endWidth - waistHalfWidth) * flare;
        const Vec2 centre = lerpVec2(start, end, t);
        leftEdge.push_back(centre + side * halfWidth);
        rightEdge.push_back(centre - side * halfWidth);
    }
    // Walk down one side and back up the other to make a closed outline.
    std::reverse(rightEdge.begin(), rightEdge.end());
    appendPoints(leftEdge, rightEdge);
    return leftEdge;
}

/** @brief A "D"-shaped bowl: flat left side at x0, round right side bulging past xFlat. */
Contour makeBowl(double x0, double top, double bottom, double xFlat, double bulge) {
    Contour bowl = {{x0, top}};
    appendPoints(bowl, sampleCubicBezier({xFlat, top}, {xFlat + bulge, top}, {xFlat + bulge, bottom}, {xFlat, bottom}, 24));
    bowl.push_back({x0, bottom});
    return bowl;
}

/** @brief A teardrop counter, pointed at the top (a closed Catmull-Rom spline). */
Contour makeTeardrop(double centreX, double top, double height, double halfWidth) {
    return sampleCatmullRomSpline({{centreX, top},
                                   {centreX + halfWidth * 0.55, top + height * 0.45},
                                   {centreX + halfWidth, top + height * 0.78},
                                   {centreX, top + height},
                                   {centreX - halfWidth, top + height * 0.78},
                                   {centreX - halfWidth * 0.55, top + height * 0.45}},
                                  8, true);
}

/**
 * @brief A superellipse |x/a|^n + |y/b|^n = 1 - a rounded rectangle when n > 2.
 *        Parametric form: x = a sgn(cos t)|cos t|^(2/n), y = b sgn(sin t)|sin t|^(2/n).
 */
Contour makeSuperellipse(const Vec2& centre, double radiusX, double radiusY, double exponent) {
    Contour shape;
    const int sampleCount = 64;
    for (int sample = 0; sample < sampleCount; ++sample) {
        const double angle = 2.0 * pi * sample / sampleCount;
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);
        shape.push_back({centre.x + radiusX * std::copysign(std::pow(std::fabs(cosine), 2.0 / exponent), cosine),
                         centre.y + radiusY * std::copysign(std::pow(std::fabs(sine), 2.0 / exponent), sine)});
    }
    return shape;
}

// ---------------------------------------------------------------------------
// Letter canvas: maps unit space onto the supersampled coverage mask
// ---------------------------------------------------------------------------

/** @brief Draws unit-space shapes into one letter's coverage mask. */
class LetterCanvas {
public:
    LetterCanvas(CoverageMask& maskValue, const Vec2& originValue, double scaleValue)
        : mask(maskValue), origin(originValue), scale(scaleValue) {}

    /** @brief Fill a closed unit-space contour into the mask (scan-line fill). */
    void add(const Contour& contour) { fillPolygonScanLine(toMask(contour), mask.makeSpanWriter(true)); }

    /** @brief Cut a closed unit-space contour out of the mask - a hole / counter. */
    void erase(const Contour& contour) { fillPolygonScanLine(toMask(contour), mask.makeSpanWriter(false)); }

    /** @brief Add a stroke whose radius tapers linearly (radii in unit space). */
    void addTaperedStroke(const Contour& centreLine, double startRadius, double endRadius) {
        fillTaperedPolyline(toMask(centreLine), startRadius * scale, endRadius * scale, mask.makeSpanWriter(true));
    }

    /** @brief Add a filled circle (unit-space centre and radius). */
    void addCircle(const Vec2& centre, double radius) {
        const Vec2 maskCentre = origin + centre * scale;
        rasterizeCircleFilled(roundToInt(maskCentre.x), roundToInt(maskCentre.y), roundToInt(radius * scale),
                              mask.makeSpanWriter(true));
    }

private:
    Contour toMask(const Contour& contour) const {
        Contour mapped;
        mapped.reserve(contour.size());
        for (const Vec2& point : contour) mapped.push_back(origin + point * scale);
        return mapped;
    }

    CoverageMask& mask;
    Vec2 origin;
    double scale;
};

// ---------------------------------------------------------------------------
// The letters
// ---------------------------------------------------------------------------

void buildLetterA(LetterCanvas& canvas) {
    // Bell-shaped outer outline with concave sides, an arch between the legs...
    Contour outline = {{0.0, 1.0}};
    appendPoints(outline, sampleCubicBezier({0.0, 1.0}, {0.17, 0.90}, {0.25, 0.30}, {0.29, 0.0}, 16));
    appendPoints(outline, sampleCubicBezier({0.49, 0.0}, {0.53, 0.30}, {0.61, 0.90}, {0.78, 1.0}, 16));
    outline.push_back({0.53, 1.0});
    appendPoints(outline, sampleCubicBezier({0.53, 1.0}, {0.51, 0.80}, {0.45, 0.72}, {0.39, 0.72}, 10));
    appendPoints(outline, sampleCubicBezier({0.39, 0.72}, {0.33, 0.72}, {0.27, 0.80}, {0.25, 1.0}, 10));
    canvas.add(outline);
    // ...and a teardrop counter.
    canvas.erase(makeTeardrop(0.39, 0.24, 0.33, 0.075));
}

void buildLetterS(LetterCanvas& canvas) {
    // The spine is a Catmull-Rom spline; the stroke swells toward the middle and thins at the ends.
    const Contour spine = sampleCatmullRomSpline({{0.58, 0.15}, {0.34, 0.04}, {0.11, 0.17}, {0.16, 0.40}, {0.33, 0.50},
                                                  {0.52, 0.61}, {0.55, 0.84}, {0.32, 0.96}, {0.08, 0.85}},
                                                 10, false);
    const std::size_t middle = spine.size() / 2;
    canvas.addTaperedStroke(Contour(spine.begin(), spine.begin() + static_cast<std::ptrdiff_t>(middle) + 1), 0.07, 0.125);
    canvas.addTaperedStroke(Contour(spine.begin() + static_cast<std::ptrdiff_t>(middle), spine.end()), 0.125, 0.07);
    // Ball terminals at both ends.
    canvas.addCircle(spine.front(), 0.085);
    canvas.addCircle(spine.back(), 0.085);
}

void buildLetterT(LetterCanvas& canvas) {
    // Top bar: flat top, ends that droop into wing-like flares.
    Contour bar = {{0.0, 0.0}, {0.74, 0.0}, {0.74, 0.22}};
    appendPoints(bar, sampleCubicBezier({0.74, 0.22}, {0.64, 0.11}, {0.52, 0.11}, {0.37, 0.11}, 12));
    appendPoints(bar, sampleCubicBezier({0.37, 0.11}, {0.22, 0.11}, {0.10, 0.11}, {0.0, 0.22}, 12));
    canvas.add(bar);
    canvas.add(makeFlaredStroke({0.37, 0.06}, {0.37, 1.0}, 0.09, 0.07, 0.15));
}

void buildLetterR(LetterCanvas& canvas) {
    canvas.add(makeFlaredStroke({0.13, 0.0}, {0.13, 1.0}, 0.13, 0.085, 0.15));
    canvas.add(makeBowl(0.13, 0.0, 0.54, 0.40, 0.32));
    canvas.erase(makeBowl(0.24, 0.13, 0.41, 0.36, 0.18));
    canvas.add(makeFlaredStroke({0.36, 0.48}, {0.64, 1.0}, 0.07, 0.06, 0.13));
}

void buildLetterW(LetterCanvas& canvas) {
    // Four flared diagonals; the wedge-shaped gaps between them form the counters.
    canvas.add(makeFlaredStroke({0.11, 0.0}, {0.31, 1.0}, 0.11, 0.07, 0.08));
    canvas.add(makeFlaredStroke({0.51, 0.04}, {0.31, 1.0}, 0.07, 0.05, 0.08));
    canvas.add(makeFlaredStroke({0.51, 0.04}, {0.71, 1.0}, 0.07, 0.05, 0.08));
    canvas.add(makeFlaredStroke({0.91, 0.0}, {0.71, 1.0}, 0.11, 0.07, 0.08));
}

void buildRingO(LetterCanvas& canvas) {
    // Concentric rounded-square rings: alternately add and erase shrinking superellipses.
    const Vec2 centre{0.41, 0.5};
    const double ringThickness = 0.055;
    const double ringPitch = 0.105;
    double radiusX = 0.39;
    double radiusY = 0.49;
    while (radiusX > 0.06) {
        canvas.add(makeSuperellipse(centre, radiusX, radiusY, 3.0));
        canvas.erase(makeSuperellipse(centre, radiusX - ringThickness, radiusY - ringThickness, 3.0));
        radiusX -= ringPitch;
        radiusY -= ringPitch;
    }
    canvas.addCircle(centre, 0.03);
}

void buildLetterL(LetterCanvas& canvas) {
    canvas.add(makeFlaredStroke({0.13, 0.0}, {0.13, 0.92}, 0.13, 0.085, 0.12));
    Contour foot = {{0.02, 0.82}};
    appendPoints(foot, sampleCubicBezier({0.24, 0.82}, {0.42, 0.82}, {0.56, 0.80}, {0.62, 0.66}, 12));
    foot.push_back({0.62, 1.0});
    foot.push_back({0.02, 1.0});
    canvas.add(foot);
}

void buildLetterD(LetterCanvas& canvas) {
    canvas.add(makeFlaredStroke({0.13, 0.0}, {0.13, 1.0}, 0.13, 0.085, 0.13));
    canvas.add(makeBowl(0.13, 0.0, 1.0, 0.36, 0.40));
    canvas.erase(makeBowl(0.24, 0.15, 0.85, 0.33, 0.26));
}

void buildLetter(char character, LetterCanvas& canvas) {
    switch (character) {
        case 'A': buildLetterA(canvas); break;
        case 'S': buildLetterS(canvas); break;
        case 'T': buildLetterT(canvas); break;
        case 'R': buildLetterR(canvas); break;
        case 'W': buildLetterW(canvas); break;
        case '@': buildRingO(canvas); break;
        case 'L': buildLetterL(canvas); break;
        case 'D': buildLetterD(canvas); break;
        default: break;
    }
}

/**
 * @brief The Earth "O" with the smiley face, drawn straight into the frame buffer.
 *
 * Sphere lighting on the ocean, random continent blobs (closed splines) clipped
 * to the disc by a per-pixel discard test, soft cloud wisps, then the smile.
 */
void drawEarthGlobe(FrameBuffer& frameBuffer, const Vec2& centre, double radius) {
    // Light factor for a point on the sphere (lit from the upper-left).
    auto sphereLight = [centre, radius](int x, int y) {
        const double u = (x - centre.x) / radius;
        const double v = (y - centre.y) / radius;
        const double nz = std::sqrt(std::max(0.0, 1.0 - u * u - v * v));
        return 0.35 + 0.75 * std::max(0.0, -0.45 * u - 0.45 * v + 0.77 * nz);
    };

    fillCircle(frameBuffer, roundToInt(centre.x), roundToInt(centre.y), roundToInt(radius), [&](int x, int y) {
        return scaleColor(astroPalette::earthOcean, sphereLight(x, y));
    });

    // Continents: blobs whose outline radius wobbles randomly, discarded outside the disc.
    SimpleRandom random(11u);
    const SpanCallback landWriter = [&](int y, int xStart, int xEnd) {
        for (int x = xStart; x <= xEnd; ++x) {
            if ((Vec2(x, y) - centre).length() > radius - 1.0) continue;  // fragment discard
            const Color land = lerpColor(astroPalette::earthLand, Color{170, 150, 90}, random.nextDouble() * 0.25);
            frameBuffer.setPixel(x, y, scaleColor(land, sphereLight(x, y)));
        }
    };
    const Vec2 blobCentres[4] = {{-0.35, -0.30}, {0.30, -0.05}, {-0.15, 0.40}, {0.45, 0.45}};
    for (const Vec2& offset : blobCentres) {
        Contour blob;
        for (int point = 0; point < 9; ++point) {
            const double angle = 2.0 * pi * point / 9.0;
            const double blobRadius = radius * random.nextRange(0.16, 0.32);
            blob.push_back(centre + offset * radius + Vec2(std::cos(angle), std::sin(angle)) * blobRadius);
        }
        fillPolygonScanLine(sampleCatmullRomSpline(blob, 6, true), landWriter);
    }

    // Cloud wisps.
    for (int wisp = 0; wisp < 5; ++wisp) {
        const Vec2 wispCentre = centre + Vec2(random.nextRange(-0.5, 0.5), random.nextRange(-0.5, 0.5)) * radius;
        blendSoftEllipse(frameBuffer, wispCentre.x, wispCentre.y, radius * 0.28, radius * 0.07, astroPalette::white, 0.6);
    }

    // The smiley: two tall eyes and a thick Bezier smile.
    const Color smileColor{255, 132, 36};
    for (double side : {-1.0, 1.0}) {
        fillEllipse(frameBuffer, roundToInt(centre.x + side * 0.24 * radius), roundToInt(centre.y - 0.18 * radius),
                    roundToInt(0.07 * radius), roundToInt(0.15 * radius), smileColor);
    }
    drawThickPolyline(frameBuffer,
                      sampleCubicBezier(centre + Vec2(-0.48, 0.12) * radius, centre + Vec2(-0.30, 0.62) * radius,
                                        centre + Vec2(0.30, 0.62) * radius, centre + Vec2(0.48, 0.12) * radius, 24),
                      0.05 * radius, smileColor);
    drawCircle(frameBuffer, roundToInt(centre.x), roundToInt(centre.y), roundToInt(radius), Color{8, 30, 70});
}

}  // namespace

double measureWordmark(const WordmarkStyle& style) {
    double unitWidth = 0.0;
    const auto& letters = logoLetters();
    for (std::size_t index = 0; index < letters.size(); ++index) {
        unitWidth += letters[index].advance;
        if (index + 1 < letters.size()) unitWidth += style.letterSpacing;
    }
    return unitWidth * style.letterHeight;
}

void drawWordmark(FrameBuffer& frameBuffer, const Vec2& topLeft, const WordmarkStyle& style) {
    const double letterHeight = style.letterHeight;
    const int factor = std::max(1, style.supersampleFactor);
    const double padding = 0.15;  // unit-space margin so flares are not clipped by the mask edge

    double cursor = 0.0;
    for (const LogoLetter& letter : logoLetters()) {
        const double letterLeft = topLeft.x + cursor * letterHeight;

        if (letter.kind == LetterKind::earthGlobe) {
            const double radius = letterHeight * 0.45;
            const Vec2 centre{letterLeft + letter.advance * 0.5 * letterHeight, topLeft.y + 0.5 * letterHeight};
            blendCircle(frameBuffer, roundToInt(centre.x + style.shadowOffset.x), roundToInt(centre.y + style.shadowOffset.y),
                        roundToInt(radius), astroPalette::black, style.shadowOpacity);
            drawEarthGlobe(frameBuffer, centre, radius);
        } else {
            // Mask covering the letter plus padding, in supersampled pixels.
            const int screenLeft = static_cast<int>(std::floor(letterLeft - padding * letterHeight));
            const int screenTop = static_cast<int>(std::floor(topLeft.y - padding * letterHeight));
            const int screenWidth = static_cast<int>(std::ceil((letter.advance + 2.0 * padding) * letterHeight));
            const int screenHeight = static_cast<int>(std::ceil((1.0 + 2.0 * padding) * letterHeight));
            CoverageMask mask(screenWidth * factor, screenHeight * factor);

            // Unit (0, 0) sits at the letter's top-left, offset by the padding inside the mask.
            const Vec2 maskOrigin{(letterLeft - screenLeft) * factor, (topLeft.y - screenTop) * factor};
            LetterCanvas canvas(mask, maskOrigin, letterHeight * factor);
            buildLetter(letter.character, canvas);

            // Shadow pass then colour pass, both from the same anti-aliased mask.
            const Color letterColor = letter.color;
            compositeCoverageMask(frameBuffer, mask, factor, screenLeft + roundToInt(style.shadowOffset.x),
                                  screenTop + roundToInt(style.shadowOffset.y),
                                  [](int, int) { return astroPalette::black; }, style.shadowOpacity);
            compositeCoverageMask(frameBuffer, mask, factor, screenLeft, screenTop,
                                  [letterColor](int, int) { return letterColor; });
        }
        cursor += letter.advance + style.letterSpacing;
    }
}
