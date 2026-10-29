#pragma once

#include "core/defs/UnitType.h"
#include "core/sim/Battle.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tq::dev {

// What the Japanese commander is doing with one unit, as the AI spec's debug panel lists it.
struct AiIntent {
    UnitId id = kNoUnit;
    AiRole role = AiRole::None;
    Order order;
    bool aggressive = false;
    // scoreTarget of the current Attack target, when it is still alive.
    std::optional<float> score;
    // The best-scoring living Ming unit and its score; empty when no Ming unit is alive.
    UnitId best = kNoUnit;
    float bestScore = 0.f;
};

// Fighting Japanese units in UnitId order.
std::vector<AiIntent> aiIntents(const Battle &battle, const GameDefs &defs);

// Scene id, or "#" plus UnitId for units created during the battle.
std::string unitLabel(const Battle &battle, UnitId id);
const char *roleName(AiRole role);
const char *stateName(UnitState state);
const char *orderName(Order::Type type);
const char *sideName(Side side);

// Every field of a unit as name and text, in declaration order.
std::vector<std::pair<std::string, std::string>> unitFields(const Battle &battle, UnitId id);

// A fixed window of recent samples in milliseconds.
class TimeSeries {
  public:
    static constexpr std::size_t kSize = 120;
    void push(float ms);
    float mean() const;
    float max() const;
    const float *data() const { return values_.data(); }
    int count() const { return static_cast<int>(values_.size()); }
    int offset() const { return static_cast<int>(next_); }

  private:
    std::vector<float> values_;
    std::size_t next_ = 0;
};

} // namespace tq::dev
