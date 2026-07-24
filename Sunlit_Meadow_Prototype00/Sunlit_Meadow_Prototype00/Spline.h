#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <initializer_list>

// =====================================================================
//  Spline
//  A 1D cubic-Hermite spline used to reshape noise into terrain.
//
//  Why this exists: raw fractal noise has a bell-shaped histogram —
//  most samples land near 0, so mapping it linearly to height gives
//  endless gentle hills and nothing else. Feeding the noise value
//  through a spline lets us RESHAPE that histogram: a knot with
//  dydx == 0 turns a whole band of noise values into one flat height
//  (a plateau/terrace), and a steep segment between two flat knots
//  turns a narrow band of noise values into a large height jump (a
//  cliff).
//
//  We use EXPLICIT per-knot derivatives (Hermite), not Catmull-Rom:
//  Catmull-Rom infers slopes from neighbors, which makes it impossible
//  to guarantee a dead-flat plateau — the inferred slope overshoots.
//  With Hermite, dydx == 0 at both ends of a segment *is* flat, and
//  the segment between a flat knot and a steep knot eases smoothly.
//
//  Outside [x0, xn] eval() CLAMPS and holds the end knot's y. The
//  control noise is bounded (~[-1, 1]) but fractal sums can nose past
//  that; cubic extrapolation would blow up (the cubic keeps curving),
//  so holding the end value is the only safe behavior.
// =====================================================================

struct SplineKnot {
	float x;      // control-noise value this knot sits at
	float y;      // output (for terrain: absolute world Z — Z-up!)
	float dydx;   // slope at the knot; 0 == plateau, large == cliff wall
};

class Spline {
public:
	Spline() = default;

	// Knots MUST be sorted ascending by x — eval() walks them in order
	// and the Hermite basis assumes x1 > x0. We log AND assert instead
	// of silently sorting: an unsorted list is always an authoring bug,
	// and silently reordering would hide it.
	Spline(std::initializer_list<SplineKnot> knots) : m_knots(knots) {
		for (size_t i = 1; i < m_knots.size(); i++) {
			if (m_knots[i].x <= m_knots[i - 1].x) {
				SDL_Log("[Spline] knots not sorted ascending by x (knot %zu: %f <= %f)!",
					i, m_knots[i].x, m_knots[i - 1].x);
				SDL_assert(false && "Spline knots must be sorted ascending by x");
			}
		}
	}

	float eval(float x) const {
		if (m_knots.empty())
			return 0.0f;

		// Clamp-and-hold outside the knot range (see header comment).
		if (x <= m_knots.front().x) return m_knots.front().y;
		if (x >= m_knots.back().x)  return m_knots.back().y;

		// Linear scan — shape splines have ~4-7 knots, a binary search
		// would cost more in branches than it saves.
		size_t i = 1;
		while (m_knots[i].x < x) i++;

		const SplineKnot& k0 = m_knots[i - 1];
		const SplineKnot& k1 = m_knots[i];

		// Cubic Hermite on [k0.x, k1.x] with the knots' explicit slopes.
		float h = k1.x - k0.x;
		float t = (x - k0.x) / h;
		float t2 = t * t;
		float t3 = t2 * t;

		float h00 =  2.0f * t3 - 3.0f * t2 + 1.0f;   // weight of y0
		float h10 =         t3 - 2.0f * t2 + t;      // weight of slope0
		float h01 = -2.0f * t3 + 3.0f * t2;          // weight of y1
		float h11 =         t3 -        t2;          // weight of slope1

		return h00 * k0.y + h10 * h * k0.dydx
		     + h01 * k1.y + h11 * h * k1.dydx;
	}

	bool empty() const { return m_knots.empty(); }

	// TODO worldgen: a knot's y could later become a nested Spline
	// evaluated on a SECOND control noise (Minecraft's Erosion /
	// Peaks&Valleys insets work exactly like this). Not implemented —
	// the single-control pipeline has to prove itself first.

private:
	std::vector<SplineKnot> m_knots;
};
