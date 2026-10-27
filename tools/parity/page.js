const LABELS = {
    李如松: "li_rusong",
    家丁左營: "jiading_left",
    家丁右營: "jiading_right",
    遼騎左營: "liaodong_left",
    遼騎中營: "liaodong_center",
    遼騎右營: "liaodong_right",
    立花長槍隊: "van_yari",
    立花鐵砲左隊: "van_teppo_left",
    立花鐵砲右隊: "van_teppo_right",
    立花武士隊: "van_samurai",
    立花宗茂: "tachibana",
    小早川隆景: "kobayakawa",
    本隊鐵砲左隊: "main_teppo_left",
    本隊鐵砲右隊: "main_teppo_right",
    本隊長槍左隊: "main_yari_left",
    本隊長槍右隊: "main_yari_right",
    本隊武士隊: "main_samurai",
    左翼武士隊: "wing_samurai_left",
    右翼武士隊: "wing_samurai_right",
    楊元左營: "yang_left",
    楊元右營: "yang_right",
};

function sim() {
    return window.__S();
}

function tagScenes() {
    for (const unit of sim().units) {
        if (unit.scene) {
            continue;
        }
        const scene = LABELS[unit.label];
        if (scene) {
            unit.scene = scene;
        } else if (!unit.origin && !String(unit.label).includes("·")) {
            throw new Error("未知单位 " + unit.label);
        }
    }
}

function ident(unit) {
    if (!unit) {
        return { id: "", seq: 0 };
    }
    let id = unit.scene || "";
    if (unit.origin) {
        const origin = sim().units.find((other) => other.id === unit.origin);
        if (origin && origin.scene) {
            id = origin.scene;
        } else if (!id) {
            id = "#" + unit.origin;
        }
    } else if (!id) {
        id = "#" + unit.id;
    }
    return { id, seq: unit.fragSeq || 0 };
}

function stateName(state) {
    if (state === "steady") return "Steady";
    if (state === "wavering") return "Wavering";
    if (state === "broken") return "Broken";
    if (state === "routing") return "Routing";
    return "Gone";
}

function unitRow(unit) {
    const name = ident(unit);
    const attack = unit.order && unit.order.type === "attack" && unit.order.target;
    const target = attack ? ident(unit.order.target) : { id: "", seq: 0 };
    const order = unit.order && unit.order.type === "attack" ? "Attack" : unit.order && unit.order.type === "move" ? "Move" : "Hold";
    const ranged = !!(unit.type && unit.type.range);
    return {
        id: name.id,
        seq: name.seq,
        x: unit.x,
        y: unit.y,
        facing: unit.f,
        men: unit.men,
        morale: unit.morale,
        state: stateName(unit.state),
        order,
        target: target.id,
        targetSeq: target.seq,
        still: unit.stillT * 8,
        reload: ranged ? unit.reloadT * 8 : 0,
        kx: (unit.kx || 0) / 8,
        ky: (unit.ky || 0) / 8,
    };
}

window.__boot = function (options) {
    window.__noAI = !!options.noAI;
    window.__pcg = makePcg(options.seed);
    Math.random = function () {
        return window.__pcg.uniform();
    };
    const note = { warn: null, phase2: null, phase3: null, reinforce: null };
    window.__ev = function (type, a, b, value) {
        const state = sim();
        if (!state._ev) {
            state._ev = [];
        }
        state._ev.push({ type, a, b, value: value ?? 0 });
    };
    const pending = [];
    let phase3At = 0;
    let withdrew = false;
    let samuraiOrdered = false;
    const width = 880;
    const height = 1100;
    const edge = 12;
    function byScene(id) {
        return sim().units.find((unit) => unit.scene === id);
    }
    function isFighting(unit) {
        return !!unit && (unit.state === "steady" || unit.state === "wavering");
    }
    function isAlive(unit) {
        return !!unit && unit.state !== "gone";
    }
    window.__applyPending = function (executing) {
        for (const cmd of pending) {
            if (cmd.tick !== executing) {
                continue;
            }
            if (cmd.type === "attack") {
                const target = byScene(cmd.target);
                if (!target || !isAlive(target) || target.side === "ming") {
                    continue;
                }
                for (const id of cmd.scenes) {
                    const unit = byScene(id);
                    if (unit && unit.side === "ming" && isFighting(unit)) {
                        unit.order = { type: "attack", target };
                    }
                }
            } else if (cmd.type === "move-unit") {
                const unit = cmd.unit;
                if (!unit || unit.side !== "ming" || !isFighting(unit)) {
                    continue;
                }
                const tx = Math.min(width - edge, Math.max(edge, cmd.x));
                const ty = Math.min(height - edge, Math.max(edge, cmd.y));
                unit.order = { type: "move", tx, ty, face: Math.atan2(cmd.y - unit.y, cmd.x - unit.x) };
            }
        }
        for (let i = pending.length - 1; i >= 0; i--) {
            if (pending[i].tick === executing) {
                pending.splice(i, 1);
            }
        }
    };
    function submit(cmd) {
        cmd.tick = (sim().tick || 0) + 1;
        pending.push(cmd);
    }
    function policy() {
        if (options.policy !== "scripted") {
            return;
        }
        const tick = sim().tick || 0;
        if (tick === 0) {
            submit({ type: "attack", scenes: ["liaodong_right"], target: "van_teppo_right" });
            submit({ type: "attack", scenes: ["jiading_left"], target: "van_teppo_left" });
        }
        if (!samuraiOrdered && tick >= 300) {
            const riders = byScene("jiading_right");
            const samurai = byScene("van_samurai");
            if (riders && samurai && isAlive(samurai) && isFighting(riders)) {
                submit({ type: "attack", scenes: ["jiading_right"], target: "van_samurai" });
                samuraiOrdered = true;
            }
        }
        if (phase3At > 0 && !withdrew && tick + 1 >= phase3At + 300) {
            for (const unit of sim().units) {
                if (unit.side === "ming" && isFighting(unit)) {
                    pending.push({ tick: tick + 1, type: "move-unit", unit, x: 440, y: 20 });
                }
            }
            withdrew = true;
        }
    }
    function absorb(events) {
        const tick = sim().tick;
        for (const event of events) {
            if (event.type === "ScoutWarning" && note.warn == null) note.warn = tick;
            if (event.type === "PhaseChanged" && event.value === 2 && note.phase2 == null) note.phase2 = tick;
            if (event.type === "PhaseChanged" && event.value === 3 && note.phase3 == null) note.phase3 = tick;
            if (event.type === "Reinforcement" && note.reinforce == null) note.reinforce = tick;
        }
    }
    function snapshot(wantTrace) {
        tagScenes();
        const state = sim();
        const events = state._ev || [];
        absorb(events);
        let text = "";
        if (wantTrace) {
            for (const event of events) {
                const a = ident(event.a);
                const b = ident(event.b);
                text +=
                    JSON.stringify({
                        kind: "event",
                        tick: state.tick,
                        type: event.type,
                        a: a.id,
                        aSeq: a.seq,
                        b: b.id,
                        bSeq: b.seq,
                        value: event.value ?? 0,
                    }) + "\n";
            }
            const units = state.units.map(unitRow).sort((left, right) => {
                if (left.id !== right.id) return left.id < right.id ? -1 : 1;
                return left.seq - right.seq;
            });
            text += JSON.stringify({ kind: "state", tick: state.tick, units }) + "\n";
        }
        state._ev = [];
        return text;
    }
    window.__f.newGame();
    const opened = sim();
    opened.tick = 0;
    opened._ev = [];
    opened.soundOn = false;
    opened.paused = true;
    tagScenes();
    window.__run = async function (maxTicks, wantTrace) {
        let buffered = snapshot(wantTrace);
        let steps = 0;
        while ((sim().tick || 0) < maxTicks && !sim().ended) {
            policy();
            window.__step(0.1);
            if (phase3At === 0 && sim().phase >= 3) {
                phase3At = sim().tick;
            }
            buffered += snapshot(wantTrace);
            steps += 1;
            if (wantTrace && steps % 100 === 0) {
                await window.__dump(buffered);
                buffered = "";
            }
        }
        if (wantTrace && buffered) {
            await window.__dump(buffered);
        }
        const state = sim();
        return {
            endTick: state.tick || 0,
            endKind: state.ended ? state.endKind || "timeout" : "timeout",
            grade: state.ended ? state.endGradeName || "none" : "none",
            rate: state.ended ? state.endRate || 0 : 0,
            warn: note.warn,
            phase2: note.phase2,
            phase3: note.phase3,
            reinforce: note.reinforce,
            mingLost: state.stats.lost.ming,
            jpLost: state.stats.lost.jp,
            mingBroken: state.stats.broken.ming,
            jpBroken: state.stats.broken.jp,
            impacts: state.stats.impacts,
            best: state.stats.best,
        };
    };
};
