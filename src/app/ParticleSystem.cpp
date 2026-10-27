#include "app/ParticleSystem.h"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>

namespace tq {
namespace {

sf::Color lerp(sf::Color from, sf::Color to, float t) {
    const auto mix = [t](std::uint8_t a, std::uint8_t b) {
        return static_cast<std::uint8_t>(a + (static_cast<float>(b) - static_cast<float>(a)) * t);
    };
    return {mix(from.r, to.r), mix(from.g, to.g), mix(from.b, to.b), mix(from.a, to.a)};
}

} // namespace

ParticleSystem::ParticleSystem() : rng_(987654u) {}

void ParticleSystem::burst(sf::Vector2f pos, int count, float speed, float life, float size, sf::Color color,
                           sf::Color endColor, const sf::Vector2f *dir, float spreadRad) {
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    const float base = dir != nullptr ? std::atan2(dir->y, dir->x) : 0.f;
    for (int i = 0; i < count && particles_.size() < kMaxParticles; ++i) {
        const float angle = base + (unit(rng_) - 0.5f) * spreadRad;
        const float velocity = speed * (0.5f + 0.7f * unit(rng_));
        Particle particle;
        const float jitter = size * (unit(rng_) * 2.f - 1.f);
        const float jitterY = size * (unit(rng_) * 2.f - 1.f);
        particle.pos = pos + sf::Vector2f{jitter, jitterY};
        particle.vel = {std::cos(angle) * velocity, std::sin(angle) * velocity};
        particle.maxLife = particle.life = life * (0.7f + 0.6f * unit(rng_));
        particle.size = size * (0.6f + 0.6f * unit(rng_));
        particle.color = color;
        particle.drag = 0.15f;
        particles_.push_back(particle);
        endColors_.push_back(endColor);
    }
}

void ParticleSystem::update(float elapsed) {
    for (std::size_t i = 0; i < particles_.size();) {
        Particle &particle = particles_[i];
        particle.life -= elapsed;
        if (particle.life <= 0.f) {
            particles_[i] = particles_.back();
            endColors_[i] = endColors_.back();
            particles_.pop_back();
            endColors_.pop_back();
            continue;
        }
        const float keep = std::pow(particle.drag, elapsed);
        particle.vel *= keep;
        particle.pos += particle.vel * elapsed;
        i += 1;
    }
}

void ParticleSystem::draw(sf::RenderTarget &target) const {
    if (particles_.empty()) {
        return;
    }
    sf::VertexArray triangles(sf::PrimitiveType::Triangles, particles_.size() * 6);
    for (std::size_t i = 0; i < particles_.size(); ++i) {
        const Particle &particle = particles_[i];
        const float t = 1.f - particle.life / particle.maxLife;
        const sf::Color color = lerp(particle.color, endColors_[i], t);
        const float half = particle.size * (1.f - 0.6f * t);
        const sf::Vector2f pos = particle.pos;
        const sf::Vertex quad[4] = {
            {{pos.x - half, pos.y - half}, color},
            {{pos.x + half, pos.y - half}, color},
            {{pos.x + half, pos.y + half}, color},
            {{pos.x - half, pos.y + half}, color},
        };
        triangles[i * 6 + 0] = quad[0];
        triangles[i * 6 + 1] = quad[1];
        triangles[i * 6 + 2] = quad[2];
        triangles[i * 6 + 3] = quad[0];
        triangles[i * 6 + 4] = quad[2];
        triangles[i * 6 + 5] = quad[3];
    }
    target.draw(triangles);
}

void ParticleSystem::clear() {
    particles_.clear();
    endColors_.clear();
}

} // namespace tq
