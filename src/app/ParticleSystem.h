#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <random>
#include <vector>

namespace tq {

// 从框架搬来的粒子批。SFML 3 没有 Quads，每个粒子画成两个三角形。
class ParticleSystem {
  public:
    struct Particle {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life = 0.f;
        float maxLife = 0.f;
        float size = 1.f;
        sf::Color color;
        float drag = 0.15f;
    };

    ParticleSystem();

    void burst(sf::Vector2f pos, int count, float speed, float life, float size, sf::Color color, sf::Color endColor,
               const sf::Vector2f *dir = nullptr, float spreadRad = 6.2831853f);
    void update(float elapsed);
    void draw(sf::RenderTarget &target) const;
    void clear();
    std::size_t count() const { return particles_.size(); }

  private:
    std::vector<Particle> particles_;
    std::vector<sf::Color> endColors_;
    std::mt19937 rng_;
    static constexpr std::size_t kMaxParticles = 4000;
};

} // namespace tq
