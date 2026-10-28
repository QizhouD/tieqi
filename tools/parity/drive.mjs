import fs from "fs";
import os from "os";
import path from "path";
import { fileURLToPath, pathToFileURL } from "url";
import { chromium } from "playwright";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../..");
const sourceHtml = path.join(root, "reference", "prototype", "byeokjegwan.html");
const anchors = JSON.parse(fs.readFileSync(path.join(here, "anchors.json"), "utf8"));

function mustReplace(text, from, to, label) {
    const index = text.indexOf(from);
    if (index < 0) {
        throw new Error("patch did not match: " + label);
    }
    if (text.indexOf(from, index + from.length) >= 0) {
        throw new Error("patch is not unique: " + label);
    }
    return text.slice(0, index) + to + text.slice(index + from.length);
}

export function patchPrototype(html, phase) {
    const anchored = (value, label) => {
        const patch = anchors.patches[label];
        return mustReplace(value, patch.from, patch.to, label);
    };
    let text = html.replace(/\r\n/g, "\n");
    text = mustReplace(
        text,
        "'use strict';\n",
        "'use strict';\nconst __nativeRandom = Math[\"random\"].bind(Math);\nfunction __vr(){ return __nativeRandom(); }\n",
        "rng-split",
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
        throw new Error("expected 5 simulation Math.random sites, found " + (leftover ? leftover.length : 0));
    }
    text = mustReplace(
        text,
        "function step(dt){\n  S.t += dt;\n  phaseCheck();\n  S.aiacc += dt; if (S.aiacc >= 1){ S.aiacc -= 1; aiThink(); }\n  for (const u of S.units) if (alive(u)) moveUnit(u, dt);\n  contactsStep();\n  S.cacc += dt; if (S.cacc >= 0.5){ S.cacc -= 0.5; combatTick(); }\n  if (!S.ended) checkEnd();\n}",
        "function step(dt){\n  const executing = (S.tick || 0) + 1;\n  if (window.__applyPending) window.__applyPending(executing);\n  S.t += dt;\n  S.tick = executing;\n  if (S.phase === 0) {\n    S.phase = 1;\n    if (window.__ev) window.__ev('PhaseChanged', null, null, 1);\n  }\n  phaseCheck();\n  if (S.tick % 10 === 0 && !window.__noAI) aiThink();\n  for (const u of S.units) if (alive(u)) moveUnit(u, dt);\n  contactsStep();\n  if (S.tick % 5 === 0) combatTick();\n  if (!S.ended) checkEnd();\n}",
        "step",
    );
    text = anchored(text, "breakaway");
    text = anchored(text, "impact");
    text = mustReplace(
        text,
        "  kill(t, t.type.men*0.015);\n  if (fighting(t)) t.morale -= 8;",
        "  kill(t, t.type.men*0.015);\n  if (window.__ev) window.__ev('ThreeEyedVolley', u, t, 0);\n  if (fighting(t)) t.morale -= 8;",
        "three-eyed",
    );
    text = anchored(text, "broken");
    text = anchored(text, "momentum");
    text = mustReplace(
        text,
        "      charging: false, cur: 0, momentum: 0, everBroken: true, frag: true, origin, baseLabel,",
        "      charging: false, cur: 0, momentum: 0, everBroken: true, frag: true, origin, baseLabel, fragSeq: k,",
        "fragment-seq",
    );
    text = mustReplace(
        text,
        "    sizeFrag(p);\n  });\n  const col = u.side === 'ming' ? C.cin : C.indigo;",
        "    sizeFrag(p);\n  });\n  if (n > 1 && window.__ev) window.__ev('Shattered', u, null, n);\n  const col = u.side === 'ming' ? C.cin : C.indigo;",
        "shatter",
    );
    text = mustReplace(
        text,
        "  u.reloadT = u.type.reload + Math.random()*0.8;\n",
        "  u.reloadT = u.type.reload + Math.random()*0.8;\n  if (window.__ev) window.__ev('TeppoVolley', u, best, 0);\n",
        "teppo",
    );
    text = anchored(text, "rallied");
    text = anchored(text, "merged");
    text = mustReplace(
        text,
        "function disperse(u){\n  u.state = 'gone';",
        "function disperse(u){\n  u.state = 'gone';\n  if (window.__ev) window.__ev('Dispersed', u, null, 0);",
        "dispersed",
    );
    text = mustReplace(
        text,
        "function fled(u){\n  u.state = 'gone'; u.fled = true;",
        "function fled(u){\n  u.state = 'gone'; u.fled = true;\n  if (window.__ev) window.__ev('Fled', u, null, 0);",
        "fled",
    );
    text = mustReplace(
        text,
        "function withdraw(u){\n  u.state = 'gone'; u.exited = true; S.exitedMen += u.men;",
        "function withdraw(u){\n  u.state = 'gone'; u.exited = true; S.exitedMen += u.men;\n  if (window.__ev) window.__ev('Withdrawn', u, null, 0);",
        "withdrawn",
    );
    text = mustReplace(
        text,
        "function generalDown(u){\n  u.state = 'gone'; u.dead = true;",
        "function generalDown(u){\n  u.state = 'gone'; u.dead = true;\n  if (window.__ev) window.__ev('GeneralDown', u, null, 0);",
        "general-down",
    );
    text = mustReplace(
        text,
        "      S.warned = true; S.mainAt = S.t + 20;\n",
        "      S.warned = true; S.mainAt = S.t + 20;\n      if (window.__ev) window.__ev('ScoutWarning', null, null, 0);\n",
        "scout",
    );
    text = anchored(text, "phase2");
    text = anchored(text, "assault");
    text = anchored(text, "reinforcement");
    text = anchored(text, "battle-end");
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
    text = mustReplace(text, "\nrequestAnimationFrame(frame);\n})();", "\n})();", "stop-loop");
    text = mustReplace(
        text,
        "\n})();",
        "\nwindow.__S = () => S; window.__step = step;\nwindow.__f = { PADDIES, HILLS, ROAD, TYPES, chargeCalc, meleeCalc, sideOf, terrainAt, capOf, meleePower, pairMul, previewCalc, add, newGame, sizeFrag, startBattle, draw, resize, buildMap };\n})();",
        "export-hooks",
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
                throw new Error("missing value for " + flag);
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
        else throw new Error("unknown argument " + flag);
    }
    if (out.policy !== "passive" && out.policy !== "scripted") {
        throw new Error("policy must be passive or scripted");
    }
    if (out.phase !== "float" && out.phase !== "tick") {
        throw new Error("phase must be float or tick");
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
    throw new Error("Playwright Chromium is missing. Run npx playwright install chromium in tools/parity");
}

export async function openPrototype(phase) {
    const work = path.join(here, "work");
    fs.mkdirSync(work, { recursive: true });
    const html = patchPrototype(fs.readFileSync(sourceHtml, "utf8"), phase);
    const patched = path.join(work, phase === "tick" ? "byeokjegwan-tick.html" : "byeokjegwan.html");
    fs.writeFileSync(patched, html);
    const pcg = fs.readFileSync(path.join(here, "pcg.mjs"), "utf8").replace(/^export /gm, "");
    const pageScript =
        "const ANCHORS = " + JSON.stringify(anchors.labels) + ";\n" + fs.readFileSync(path.join(here, "page.js"), "utf8");
    const browser = await chromium.launch({ headless: true, executablePath: findChrome() });
    const page = await browser.newPage();
    page.on("pageerror", (error) => {
        console.error("prototype page error", error);
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
