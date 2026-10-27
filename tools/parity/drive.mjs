import fs from "fs";
import os from "os";
import path from "path";
import { fileURLToPath, pathToFileURL } from "url";
import { chromium } from "playwright";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../..");
const sourceHtml = path.join(root, "reference", "prototype", "byeokjegwan.html");

function mustReplace(text, from, to, label) {
    const index = text.indexOf(from);
    if (index < 0) {
        throw new Error("补丁对不上：" + label);
    }
    if (text.indexOf(from, index + from.length) >= 0) {
        throw new Error("补丁不唯一：" + label);
    }
    return text.slice(0, index) + to + text.slice(index + from.length);
}

export function patchPrototype(html, phase) {
    let text = html.replace(/\r\n/g, "\n");
    text = mustReplace(
        text,
        "'use strict';\n",
        "'use strict';\nconst __nativeRandom = Math[\"random\"].bind(Math);\nfunction __vr(){ return __nativeRandom(); }\n",
        "随机数分流",
    );
    text = text.replaceAll("Math.random", "__vr");
    const simSites = [
        ["reloadT: __vr()*2", "reloadT: Math.random()*2"],
        ["0.9 + __vr()*0.2", "0.9 + Math.random()*0.2"],
        ["u.type.reload + __vr()*0.8", "u.type.reload + Math.random()*0.8"],
        ["(__vr()-0.5)*0.3", "(Math.random()-0.5)*0.3"],
        ["150 + __vr()*80", "150 + Math.random()*80"],
    ];
    for (const [from, to] of simSites) {
        text = mustReplace(text, from, to, from);
    }
    const leftover = text.match(/Math\.random/g);
    if (!leftover || leftover.length !== 5) {
        throw new Error("模拟随机数应剩 5 处，实际 " + (leftover ? leftover.length : 0));
    }
    text = mustReplace(
        text,
        "function step(dt){\n  S.t += dt;\n  phaseCheck();\n  S.aiacc += dt; if (S.aiacc >= 1){ S.aiacc -= 1; aiThink(); }\n  for (const u of S.units) if (alive(u)) moveUnit(u, dt);\n  contactsStep();\n  S.cacc += dt; if (S.cacc >= 0.5){ S.cacc -= 0.5; combatTick(); }\n  if (!S.ended) checkEnd();\n}",
        "function step(dt){\n  const executing = (S.tick || 0) + 1;\n  if (window.__applyPending) window.__applyPending(executing);\n  S.t += dt;\n  S.tick = executing;\n  if (S.phase === 0) {\n    S.phase = 1;\n    if (window.__ev) window.__ev('PhaseChanged', null, null, 1);\n  }\n  phaseCheck();\n  if (S.tick % 10 === 0 && !window.__noAI) aiThink();\n  for (const u of S.units) if (alive(u)) moveUnit(u, dt);\n  contactsStep();\n  if (S.tick % 5 === 0) combatTick();\n  if (!S.ended) checkEnd();\n}",
        "step",
    );
    text = mustReplace(
        text,
        "  u.morale -= 5;\n  logEv(`${u.label} 脫離接戰`, u.side);",
        "  u.morale -= 5;\n  if (window.__ev) window.__ev('Breakaway', u, null, 0);\n  logEv(`${u.label} 脫離接戰`, u.side);",
        "脱离",
    );
    text = mustReplace(
        text,
        "  logEv(`${a.label} 自${SIDE_TXT[c.side]}衝擊 ${t.label}，衝擊 ×${r.toFixed(1)}${c.pikeFront ? '，撞上槍陣' : ''}`, a.side, breaks || c.pikeFront);",
        "  if (window.__ev) window.__ev('Impact', a, t, r);\n  logEv(`${a.label} 自${SIDE_TXT[c.side]}衝擊 ${t.label}，衝擊 ×${r.toFixed(1)}${c.pikeFront ? '，撞上槍陣' : ''}`, a.side, breaks || c.pikeFront);",
        "冲击",
    );
    text = mustReplace(
        text,
        "  kill(t, t.type.men*0.015);\n  if (fighting(t)) t.morale -= 8;",
        "  kill(t, t.type.men*0.015);\n  if (window.__ev) window.__ev('ThreeEyedVolley', u, t, 0);\n  if (fighting(t)) t.morale -= 8;",
        "三眼铳",
    );
    text = mustReplace(
        text,
        "  u.morale = Math.min(u.morale, 25);\n  logEv(`${u.label} ${reason === '破陣' ? '陣腳被衝破，' : ''}崩潰${parts > 1 ? '，散作' + '兩三'[parts-2] + '股' : ''}`, u.side === 'ming' ? 'jp' : 'ming', true);",
        "  u.morale = Math.min(u.morale, 25);\n  if (window.__ev) window.__ev('Broken', u, null, parts);\n  logEv(`${u.label} ${reason === '破陣' ? '陣腳被衝破，' : ''}崩潰${parts > 1 ? '，散作' + '兩三'[parts-2] + '股' : ''}`, u.side === 'ming' ? 'jp' : 'ming', true);",
        "崩溃",
    );
    text = mustReplace(
        text,
        "    v.momentum = Math.min(3, v.momentum + add_); v.momT = 0;\n    floatText(v.x, v.y-26, '氣勢', C.indigo, 18);",
        "    v.momentum = Math.min(3, v.momentum + add_); v.momT = 0;\n    if (window.__ev) window.__ev('MomentumGained', v, null, add_);\n    floatText(v.x, v.y-26, '氣勢', C.indigo, 18);",
        "气势",
    );
    text = mustReplace(
        text,
        "      charging: false, cur: 0, momentum: 0, everBroken: true, frag: true, origin, baseLabel,",
        "      charging: false, cur: 0, momentum: 0, everBroken: true, frag: true, origin, baseLabel, fragSeq: k,",
        "残部序号",
    );
    text = mustReplace(
        text,
        "    sizeFrag(p);\n  });\n  const col = u.side === 'ming' ? C.cin : C.indigo;",
        "    sizeFrag(p);\n  });\n  if (n > 1 && window.__ev) window.__ev('Shattered', u, null, n);\n  const col = u.side === 'ming' ? C.cin : C.indigo;",
        "散开",
    );
    text = mustReplace(
        text,
        "  u.reloadT = u.type.reload + Math.random()*0.8;\n",
        "  u.reloadT = u.type.reload + Math.random()*0.8;\n  if (window.__ev) window.__ev('TeppoVolley', u, best, 0);\n",
        "铁炮",
    );
    text = mustReplace(
        text,
        "          logEv(`${u.label} 重新集結`, u.side);",
        "          if (window.__ev) window.__ev('Rallied', u, null, 0);\n          logEv(`${u.label} 重新集結`, u.side);",
        "集结",
    );
    text = mustReplace(
        text,
        "    logEv(`${a.baseLabel} ${b.frag ? '收攏殘部' : '合兵'}，兵 ${Math.round(tot)}`, a.side);",
        "    a.fragSeq = 0;\n    if (window.__ev) window.__ev('Merged', a, b, tot);\n    logEv(`${a.baseLabel} ${b.frag ? '收攏殘部' : '合兵'}，兵 ${Math.round(tot)}`, a.side);",
        "合并",
    );
    text = mustReplace(
        text,
        "function disperse(u){\n  u.state = 'gone';",
        "function disperse(u){\n  u.state = 'gone';\n  if (window.__ev) window.__ev('Dispersed', u, null, 0);",
        "溃散",
    );
    text = mustReplace(
        text,
        "function fled(u){\n  u.state = 'gone'; u.fled = true;",
        "function fled(u){\n  u.state = 'gone'; u.fled = true;\n  if (window.__ev) window.__ev('Fled', u, null, 0);",
        "逃离",
    );
    text = mustReplace(
        text,
        "function withdraw(u){\n  u.state = 'gone'; u.exited = true; S.exitedMen += u.men;",
        "function withdraw(u){\n  u.state = 'gone'; u.exited = true; S.exitedMen += u.men;\n  if (window.__ev) window.__ev('Withdrawn', u, null, 0);",
        "撤出",
    );
    text = mustReplace(
        text,
        "function generalDown(u){\n  u.state = 'gone'; u.dead = true;",
        "function generalDown(u){\n  u.state = 'gone'; u.dead = true;\n  if (window.__ev) window.__ev('GeneralDown', u, null, 0);",
        "阵亡",
    );
    text = mustReplace(
        text,
        "      S.warned = true; S.mainAt = S.t + 20;\n",
        "      S.warned = true; S.mainAt = S.t + 20;\n      if (window.__ev) window.__ev('ScoutWarning', null, null, 0);\n",
        "斥候",
    );
    text = mustReplace(
        text,
        "  logEv('日軍本隊自礪石峴壓上，北口撤退路線已開', 'jp', true);",
        "  logEv('日軍本隊自礪石峴壓上，北口撤退路線已開', 'jp', true);\n  if (window.__ev) window.__ev('PhaseChanged', null, null, 2);",
        "阶段2",
    );
    text = mustReplace(
        text,
        "    logEv('小早川隆景下令總攻，日軍士氣大振', 'jp', true);\n  } else logEv('日軍本隊失去主將，總攻未能發動', 'ming', true);",
        "    if (window.__ev) window.__ev('GrandAssault', k, null, 0);\n    logEv('小早川隆景下令總攻，日軍士氣大振', 'jp', true);\n  } else {\n    if (window.__ev) window.__ev('GrandAssault', null, null, 0);\n    logEv('日軍本隊失去主將，總攻未能發動', 'ming', true);\n  }\n  if (window.__ev) window.__ev('PhaseChanged', null, null, 3);",
        "总攻",
    );
    text = mustReplace(
        text,
        "  logEv('楊元率押後的一千騎趕到，可聽調遣', 'ming', true);",
        "  logEv('楊元率押後的一千騎趕到，可聽調遣', 'ming', true);\n  if (window.__ev) window.__ev('Reinforcement', null, null, 0);",
        "杨元",
    );
    text = mustReplace(
        text,
        "  $('endGrade').textContent = grade;",
        "  const __gv = grade === '大勝' ? 1 : grade === '小勝' ? 2 : grade === '慘勝' ? 3 : 4;\n  if (window.__ev) window.__ev('BattleEnded', null, null, __gv);\n  S.endKind = kind === 'dead' ? 'commander_killed' : kind === 'win' ? 'withdrawn' : kind === 'fled' ? 'commander_fled' : kind === 'rout' ? 'routed' : 'swept';\n  S.endGradeName = __gv === 1 ? 'decisive' : __gv === 2 ? 'minor' : __gv === 3 ? 'pyrrhic' : 'defeat';\n  S.endRate = rate;\n  $('endGrade').textContent = grade;",
        "终局",
    );
    if (phase === "tick") {
        const ticks = [
            ["S.t > 45", "S.tick > 450"],
            ["S.t >= 160", "S.tick >= 1600"],
            ["S.t >= 80", "S.tick >= 800"],
            ["S.mainAt = S.t + 20", "S.mainAt = S.tick + 200"],
            ["S.t - S.phase2At >= 90", "S.tick - S.phase2At >= 900"],
            ["S.t - S.phase3At >= 25", "S.tick - S.phase3At >= 250"],
            ["S.t >= S.mainAt", "S.tick >= S.mainAt"],
            ["S.t - S.phase2At >= 20", "S.tick - S.phase2At >= 200"],
            ["S.phase2At = S.t", "S.phase2At = S.tick"],
            ["S.phase3At = S.t", "S.phase3At = S.tick"],
        ];
        for (const [from, to] of ticks) {
            text = mustReplace(text, from, to, from);
        }
    }
    text = mustReplace(text, "\nrequestAnimationFrame(frame);\n})();", "\n})();", "停掉画面循环");
    text = mustReplace(
        text,
        "\n})();",
        "\nwindow.__S = () => S; window.__step = step;\nwindow.__f = { PADDIES, HILLS, ROAD, TYPES, chargeCalc, meleeCalc, sideOf, terrainAt, capOf, meleePower, pairMul, previewCalc, add, newGame, sizeFrag, startBattle };\n})();",
        "导出",
    );
    return text;
}

function readArgs(argv) {
    const out = {
        seed: 1,
        runs: 1,
        policy: "passive",
        maxTicks: 18000,
        phase: "float",
        noAI: false,
        summary: false,
        out: "",
    };
    for (let i = 2; i < argv.length; i++) {
        const flag = argv[i];
        const next = () => {
            if (i + 1 >= argv.length) {
                throw new Error("缺少参数 " + flag);
            }
            return argv[++i];
        };
        if (flag === "--seed") out.seed = Number(next());
        else if (flag === "--runs") out.runs = Number(next());
        else if (flag === "--policy") out.policy = next();
        else if (flag === "--max-ticks") out.maxTicks = Number(next());
        else if (flag === "--phase") out.phase = next();
        else if (flag === "--out") out.out = next();
        else if (flag === "--no-ai") out.noAI = true;
        else if (flag === "--summary") out.summary = true;
        else throw new Error("未知参数 " + flag);
    }
    if (out.policy !== "passive" && out.policy !== "scripted") {
        throw new Error("策略只能是 passive 或 scripted");
    }
    if (out.phase !== "float" && out.phase !== "tick") {
        throw new Error("phase 只能是 float 或 tick");
    }
    return out;
}

function findChrome() {
    const root = path.join(process.env.LOCALAPPDATA || path.join(os.homedir(), "AppData", "Local"), "ms-playwright");
    const dirs = fs.existsSync(root) ? fs.readdirSync(root).filter((name) => name.startsWith("chromium-")) : [];
    for (const dir of dirs) {
        const exe = path.join(root, dir, "chrome-win64", "chrome.exe");
        if (fs.existsSync(exe)) {
            return exe;
        }
    }
    throw new Error("找不到 Playwright 的 Chromium。在 tools/parity 里执行 npx playwright install chromium");
}

export async function openPrototype(phase) {
    const work = path.join(here, "work");
    fs.mkdirSync(work, { recursive: true });
    const html = patchPrototype(fs.readFileSync(sourceHtml, "utf8"), phase);
    const patched = path.join(work, phase === "tick" ? "byeokjegwan-tick.html" : "byeokjegwan.html");
    fs.writeFileSync(patched, html);
    const pcg = fs.readFileSync(path.join(here, "pcg.mjs"), "utf8").replace(/^export /gm, "");
    const pageScript = fs.readFileSync(path.join(here, "page.js"), "utf8");
    const browser = await chromium.launch({ headless: true, executablePath: findChrome() });
    const page = await browser.newPage();
    page.on("pageerror", (error) => {
        console.error("原型页面错误", error);
    });
    let dumpPath = "";
    await page.exposeFunction("__dump", (text) => {
        if (dumpPath) {
            fs.appendFileSync(dumpPath, text);
        }
    });
    await page.goto(pathToFileURL(patched).href, { waitUntil: "load" });
    await page.addScriptTag({ content: pcg });
    await page.addScriptTag({ content: pageScript });
    return {
        browser,
        page,
        async battle(options) {
            dumpPath = options.out ? path.resolve(options.out) : "";
            if (dumpPath) {
                fs.mkdirSync(path.dirname(dumpPath), { recursive: true });
                fs.writeFileSync(dumpPath, "");
            }
            await page.evaluate((boot) => window.__boot(boot), {
                seed: options.seed,
                policy: options.policy,
                noAI: !!options.noAI,
            });
            return page.evaluate(
                ([maxTicks, wantTrace]) => window.__run(maxTicks, wantTrace),
                [options.maxTicks, !!options.out],
            );
        },
    };
}

async function main() {
    const args = readArgs(process.argv);
    const session = await openPrototype(args.phase);
    try {
        const rows = [];
        for (let i = 0; i < args.runs; i++) {
            const seed = args.seed + i;
            let out = "";
            if (args.out && !args.summary) {
                out = args.runs === 1 ? args.out : args.out.replace(/(\.[^.]+)?$/, `-${seed}$1`);
            }
            const summary = await session.battle({
                seed,
                policy: args.policy,
                noAI: args.noAI,
                maxTicks: args.maxTicks,
                out,
            });
            summary.seed = seed;
            summary.policy = args.policy;
            rows.push(summary);
            if (args.summary) {
                console.log(JSON.stringify(summary));
            }
        }
        if (args.summary && args.out) {
            fs.mkdirSync(path.dirname(path.resolve(args.out)), { recursive: true });
            fs.writeFileSync(args.out, rows.map((row) => JSON.stringify(row)).join("\n") + "\n");
        }
    } finally {
        await session.browser.close();
    }
}

const invoked = process.argv[1] && process.argv[1].replaceAll("\\", "/").endsWith("/drive.mjs");
if (invoked) {
    main().catch((error) => {
        console.error(error);
        process.exit(1);
    });
}
