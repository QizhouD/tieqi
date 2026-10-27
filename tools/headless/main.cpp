#include "Run.h"
#include "data/Load.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Args {
    std::string scenario;
    std::uint64_t seed = 1;
    int runs = 1;
    tq::PolicyKind policy = tq::PolicyKind::Scripted;
    bool ai = true;
    std::uint32_t maxTicks = 18000;
    std::optional<std::filesystem::path> out;
    std::optional<std::filesystem::path> replayDir;
    std::optional<std::filesystem::path> dataRoot;
    bool sawScenario = false;
    bool sawPolicy = false;
};

void usage() {
    std::cerr << "tq_headless --scenario byeokjegwan --seed 1 --runs 30 --policy scripted\n"
              << "            [--no-ai] [--max-ticks 18000] [--out runs.csv] [--replay-dir replays/]\n"
              << "            [--data-root data]\n";
}

bool parse(int argc, char **argv, Args &args) {
    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];
        auto need = [&](const char *name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "缺少参数 " << name << '\n';
                return {};
            }
            return argv[++i];
        };
        if (flag == "--scenario") {
            args.scenario = need("--scenario");
            args.sawScenario = !args.scenario.empty();
        } else if (flag == "--seed") {
            args.seed = std::stoull(need("--seed"));
        } else if (flag == "--runs") {
            args.runs = std::stoi(need("--runs"));
        } else if (flag == "--policy") {
            const std::string value = need("--policy");
            args.sawPolicy = true;
            if (value == "passive") {
                args.policy = tq::PolicyKind::Passive;
            } else if (value == "scripted") {
                args.policy = tq::PolicyKind::Scripted;
            } else {
                std::cerr << "策略只能是 passive 或 scripted\n";
                return false;
            }
        } else if (flag == "--no-ai") {
            args.ai = false;
        } else if (flag == "--max-ticks") {
            args.maxTicks = static_cast<std::uint32_t>(std::stoul(need("--max-ticks")));
        } else if (flag == "--out") {
            args.out = need("--out");
        } else if (flag == "--replay-dir") {
            args.replayDir = need("--replay-dir");
        } else if (flag == "--data-root") {
            args.dataRoot = need("--data-root");
        } else if (flag == "--help") {
            usage();
            return false;
        } else {
            std::cerr << "未知参数 " << flag << '\n';
            return false;
        }
    }
    if (!args.sawScenario || !args.sawPolicy || args.runs < 1) {
        usage();
        return false;
    }
    return true;
}

std::filesystem::path findData(const Args &args) {
    if (args.dataRoot) {
        return *args.dataRoot;
    }
    std::filesystem::path cursor = std::filesystem::current_path();
    for (int i = 0; i < 6; ++i) {
        if (std::filesystem::exists(cursor / "data" / "units.json")) {
            return cursor / "data";
        }
        if (!cursor.has_parent_path() || cursor.parent_path() == cursor) {
            break;
        }
        cursor = cursor.parent_path();
    }
    throw tq::DataError("找不到 data/units.json");
}

std::string field(const std::optional<std::uint32_t> &value) {
    if (!value) {
        return {};
    }
    return std::to_string(*value);
}

double percentile(std::vector<double> values, double portion) {
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t index = static_cast<std::size_t>(std::min(
        values.size() - 1, static_cast<std::size_t>(std::ceil(portion * static_cast<double>(values.size()))) - 1));
    return values[index];
}

} // namespace

int main(int argc, char **argv) {
    Args args;
    try {
        if (!parse(argc, argv, args)) {
            return 2;
        }
        const std::filesystem::path dataDir = findData(args);
        const tq::GameData data = tq::loadGameData(dataDir, args.scenario + ".json");
        if (args.replayDir) {
            std::filesystem::create_directories(*args.replayDir);
        }
        std::vector<tq::RunResult> results;
        results.reserve(static_cast<std::size_t>(args.runs));
        bool failed = false;
        for (int i = 0; i < args.runs; ++i) {
            tq::RunRequest request;
            request.policy = args.policy;
            request.seed = args.seed + static_cast<std::uint64_t>(i);
            request.ai = args.ai;
            request.maxTicks = args.maxTicks;
            const auto started = std::chrono::steady_clock::now();
            tq::RunResult result = tq::runBattle(data, request);
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            if (result.timeout || !result.finite || result.events <= 0 || seconds > 10.0) {
                failed = true;
            }
            if (args.policy == tq::PolicyKind::Scripted && result.impacts <= 0) {
                failed = true;
            }
            if (args.replayDir) {
                tq::saveReplay(*args.replayDir / ("seed-" + std::to_string(request.seed) + ".json"), result.replay);
            }
            results.push_back(std::move(result));
        }
        std::ostringstream csv;
        csv << "seed,policy,end_tick,end_kind,grade,saved_rate,warn_tick,phase2_tick,phase3_tick,reinforce_tick,"
               "ming_lost,jp_lost,ming_broken,jp_broken,impacts,best_impact,events\n";
        const char *policyName = args.policy == tq::PolicyKind::Passive ? "passive" : "scripted";
        for (int i = 0; i < args.runs; ++i) {
            const tq::RunResult &result = results[static_cast<std::size_t>(i)];
            csv << (args.seed + static_cast<std::uint64_t>(i)) << ',' << policyName << ',' << result.endTick << ','
                << result.endKind << ',' << result.grade << ',' << result.savedRate << ',' << field(result.warnTick)
                << ',' << field(result.phase2Tick) << ',' << field(result.phase3Tick) << ','
                << field(result.reinforceTick) << ',' << result.mingLost << ',' << result.jpLost << ','
                << result.mingBroken << ',' << result.jpBroken << ',' << result.impacts << ',' << result.bestImpact
                << ',' << result.events << '\n';
        }
        if (args.out) {
            std::ofstream out(*args.out, std::ios::binary);
            if (!out) {
                std::cerr << "无法写入 " << args.out->string() << '\n';
                return 1;
            }
            out << csv.str();
        }
        auto column = [&](auto pick) {
            std::vector<double> values;
            for (const tq::RunResult &result : results) {
                values.push_back(static_cast<double>(pick(result)));
            }
            std::cout << percentile(values, 0.05) << ' ' << percentile(values, 0.5) << ' ' << percentile(values, 0.95)
                      << '\n';
        };
        std::cout << "end_tick ";
        column([](const tq::RunResult &result) { return result.endTick; });
        std::cout << "saved_rate ";
        column([](const tq::RunResult &result) { return result.savedRate; });
        std::cout << "impacts ";
        column([](const tq::RunResult &result) { return result.impacts; });
        std::cout << "jp_broken ";
        column([](const tq::RunResult &result) { return result.jpBroken; });
        for (const tq::RunResult &result : results) {
            std::cout << result.endKind << ' ' << result.grade << '\n';
        }
        return failed ? 1 : 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
