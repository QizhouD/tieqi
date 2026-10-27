import fs from "fs";
import readline from "readline";

const POS = 1e-3;
const ANGLE = 1e-4;
const MEN = 0.05;
const STILL = 0.05;
const VALUE = 1e-3;

function near(left, right, eps) {
    return Math.abs(left - right) <= eps;
}

async function load(file) {
    const events = new Map();
    const states = new Map();
    const rl = readline.createInterface({ input: fs.createReadStream(file), crlfDelay: Infinity });
    for await (const line of rl) {
        if (!line) continue;
        const row = JSON.parse(line);
        if (row.kind === "event") {
            if (!events.has(row.tick)) events.set(row.tick, []);
            events.get(row.tick).push(row);
        } else if (row.kind === "state") {
            states.set(row.tick, row.units);
        }
    }
    return { events, states };
}

function eventText(event) {
    if (!event) return "（无）";
    const value = event.value == null ? "" : " value " + event.value;
    return `${event.type} ${event.a || "-"}#${event.aSeq} -> ${event.b || "-"}#${event.bSeq}${value}`;
}

function unitText(unit) {
    if (!unit) return "（无）";
    return `${unit.id}#${unit.seq} ${unit.state} ${unit.order}${unit.target ? "->" + unit.target + "#" + unit.targetSeq : ""} (${unit.x.toFixed(2)},${unit.y.toFixed(2)}) men ${unit.men.toFixed(2)} mor ${unit.morale.toFixed(2)} face ${unit.facing.toFixed(4)} still ${unit.still.toFixed(2)} reload ${unit.reload.toFixed(3)}`;
}

function findUnit(units, id, seq) {
    return units.find((unit) => unit.id === id && unit.seq === seq);
}

function sameEvent(left, right) {
    return (
        left.type === right.type &&
        left.a === right.a &&
        left.aSeq === right.aSeq &&
        left.b === right.b &&
        left.bSeq === right.bSeq &&
        near(left.value, right.value, VALUE)
    );
}

function unitDelta(left, right) {
    const reasons = [];
    if (!near(left.x, right.x, POS) || !near(left.y, right.y, POS)) {
        reasons.push(`位置 (${left.x.toFixed(3)},${left.y.toFixed(3)}) / (${right.x.toFixed(3)},${right.y.toFixed(3)})`);
    }
    if (!near(left.facing, right.facing, ANGLE)) reasons.push("朝向");
    if (!near(left.men, right.men, MEN)) reasons.push(`兵力 ${left.men.toFixed(3)} / ${right.men.toFixed(3)}`);
    if (!near(left.morale, right.morale, MEN)) reasons.push(`士气 ${left.morale.toFixed(3)} / ${right.morale.toFixed(3)}`);
    if (left.state !== right.state) reasons.push(`状态 ${left.state} / ${right.state}`);
    if (left.order !== right.order) reasons.push(`命令 ${left.order} / ${right.order}`);
    if (left.target !== right.target || left.targetSeq !== right.targetSeq) {
        reasons.push(`目标 ${left.target}#${left.targetSeq} / ${right.target}#${right.targetSeq}`);
    }
    if (!near(left.still, right.still, STILL)) reasons.push("停步");
    return reasons;
}

function related(proto, port, ids) {
    const lines = [];
    const seen = new Set();
    for (const key of ids) {
        if (seen.has(key)) continue;
        seen.add(key);
        const [id, seqText] = key.split("#");
        const seq = Number(seqText);
        lines.push("  原型 " + unitText(findUnit(proto, id, seq)));
        lines.push("  移植 " + unitText(findUnit(port, id, seq)));
    }
    return lines;
}

export async function compareFiles(protoFile, portFile) {
    const proto = await load(protoFile);
    const port = await load(portFile);
    const ticks = [...new Set([...proto.states.keys(), ...port.states.keys()])].sort((a, b) => a - b);
    const floats = [];
    for (const tick of ticks) {
        const protoEvents = proto.events.get(tick) || [];
        const portEvents = port.events.get(tick) || [];
        const count = Math.max(protoEvents.length, portEvents.length);
        for (let i = 0; i < count; i++) {
            const left = protoEvents[i];
            const right = portEvents[i];
            if (!left || !right || !sameEvent(left, right)) {
                return report(tick, "事件", left, right, proto, port, floats);
            }
        }
        const protoUnits = proto.states.get(tick) || [];
        const portUnits = port.states.get(tick) || [];
        const keys = new Set([
            ...protoUnits.map((unit) => unit.id + "#" + unit.seq),
            ...portUnits.map((unit) => unit.id + "#" + unit.seq),
        ]);
        for (const key of [...keys].sort()) {
            const [id, seqText] = key.split("#");
            const seq = Number(seqText);
            const left = findUnit(protoUnits, id, seq);
            const right = findUnit(portUnits, id, seq);
            if (!left || !right) {
                return report(tick, "单位", left, right, proto, port, floats, key);
            }
            const reasons = unitDelta(left, right);
            if (reasons.length) {
                const onlyPose = reasons.every((reason) => reason.startsWith("位置") || reason === "朝向");
                const pose = Math.hypot(left.x - right.x, left.y - right.y);
                if (onlyPose && pose <= 0.05 && Math.abs(left.facing - right.facing) <= 1e-3) {
                    if (floats.length < 8) {
                        floats.push(`tick ${tick} ${key} 位移 ${pose.toExponential(2)} 朝向 ${(left.facing - right.facing).toExponential(2)}`);
                    }
                    continue;
                }
                return report(tick, reasons.join("，"), left, right, proto, port, floats, key);
            }
        }
    }
    return { match: true, floats, ticks: ticks.length };
}

function report(tick, why, left, right, proto, port, floats, key) {
    const protoUnits = proto.states.get(tick) || [];
    const portUnits = port.states.get(tick) || [];
    const prevProto = proto.states.get(tick - 1) || [];
    const prevPort = port.states.get(tick - 1) || [];
    const ids = [];
    if (key) ids.push(key);
    for (const event of [left, right]) {
        if (!event || !event.type) continue;
        if (event.a) ids.push(event.a + "#" + event.aSeq);
        if (event.b) ids.push(event.b + "#" + event.bSeq);
    }
    const lines = [];
    lines.push(`第一处不一致：tick ${tick} ${why}`);
    if (left && left.type) lines.push("原型事件 " + eventText(left));
    if (right && right.type) lines.push("移植事件 " + eventText(right));
    if (left && left.men != null) lines.push("原型单位 " + unitText(left));
    if (right && right.men != null) lines.push("移植单位 " + unitText(right));
    lines.push("本 tick");
    lines.push(...related(protoUnits, portUnits, ids));
    if (prevProto.length || prevPort.length) {
        lines.push("上一 tick");
        lines.push(...related(prevProto, prevPort, ids));
    }
    return { match: false, tick, why, floats, text: lines.join("\n") };
}

async function main() {
    const [protoFile, portFile] = process.argv.slice(2);
    if (!protoFile || !portFile) {
        console.error("用法：node compare.mjs 原型.jsonl 移植.jsonl");
        process.exit(2);
    }
    const result = await compareFiles(protoFile, portFile);
    if (result.floats.length) {
        console.log("浮点尾差（未追）");
        for (const line of result.floats) console.log(line);
    }
    if (result.match) {
        console.log(`一致，共 ${result.ticks} 个 tick`);
        return;
    }
    console.log(result.text);
    process.exitCode = 1;
}

const invoked = process.argv[1] && process.argv[1].replaceAll("\\", "/").endsWith("/compare.mjs");
if (invoked) {
    main().catch((error) => {
        console.error(error);
        process.exit(1);
    });
}
