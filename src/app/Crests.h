#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

namespace tq {

// Vector crests for the Japanese sashimono (V6), drawn at run time so they stay sharp at any zoom. Shapes follow the
// standard drawings of each clan's mon, simplified to filled shapes; the author reviews them.

// Kobayakawa Takakage: hidari mitsudomoe, three comma-shaped tomoe whose tails sweep counterclockwise.
void appendMitsudomoe(sf::VertexArray &tris, sf::Vector2f centre, float radius, sf::Color ink);

// Tachibana Muneshige: naka-musubi gion mamori, two crossed amulet scrolls tied with a knot where they
// cross, each scroll ending in a cross-shaped finial.
void appendGionMamori(sf::VertexArray &tris, sf::Vector2f centre, float radius, sf::Color ink);

} // namespace tq
