import { spawn } from "child_process";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { compareFiles } from "./compare.mjs";
import { openPrototype } from "./drive.mjs";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../..");
const exe = path.join(root, "build", "dev", "tq_headless.exe");

function readArgs(argv) {
    const out = { from: 1, to: 30, phase: "float", maxTicks: 18000, policies: ["passive", "scripted"] };
    for (let i = 2; i < argv.length; i++) {
        const flag = argv[i];
        const next = () => argv[++i];
        if (flag === "--from") out.from = Number(next());
        else if (flag === "--to") out.to = Number(next());
        else if (flag === "--phase") out.phase = next();
        else if (flag === "--max-ticks") out.maxTicks = Number(next());
        else if (flag === "--policy") out.policies = [next()];
        else throw new Error("unknown argument " + flag);
    }
    return out;
}

function runPort(seed, policy, outFile, maxTicks) {
    return new Promise((resolve, reject) => {
        const child = spawn(
            exe,
            [
                "--scenario",
                "byeokjegwan",
                "--seed",
                String(seed),
                "--runs",
                "1",
                "--policy",
                policy,
                "--max-ticks",
                String(maxTicks),
                "--events-out",
                outFile,
            ],
            { cwd: root },
        );
        let text = "";
        child.stdout.on("data", (chunk) => {
            text += chunk;
        });
        child.stderr.on("data", (chunk) => {
            text += chunk;
        });
        child.on("error", reject);
        child.on("close", (code) => {
            if (code !== 0 && code !== 1) {
                reject(new Error(`tq_headless exited ${code}\n${text}`));
                return;
            }
            resolve(text);
        });
    });
}

async function main() {
    const args = readArgs(process.argv);
    const work = path.join(here, "work");
    fs.mkdirSync(work, { recursive: true });
    const session = await openPrototype(args.phase);
    const found = [];
    try {
        for (const policy of args.policies) {
            for (let seed = args.from; seed <= args.to; seed++) {
                const protoFile = path.join(work, `proto-${policy}-${seed}.jsonl`);
                const portFile = path.join(work, `port-${policy}-${seed}.jsonl`);
                process.stdout.write(`${policy} seed ${seed} … `);
                await session.battle({
                    seed,
                    policy,
                    maxTicks: args.maxTicks,
                    out: protoFile,
                });
                await runPort(seed, policy, portFile, args.maxTicks);
                const result = await compareFiles(protoFile, portFile);
                if (result.match) {
                    console.log("match");
                    fs.rmSync(protoFile, { force: true });
                    fs.rmSync(portFile, { force: true });
                    continue;
                }
                const floatNote = result.floats.length ? `, float tail ${result.floats.length}` : "";
                console.log(`tick ${result.tick} ${result.why}${floatNote}`);
                const note = path.join(work, `diff-${policy}-${seed}.txt`);
                const floatText = result.floats.length ? "float tail (not chased)\n" + result.floats.join("\n") + "\n" : "";
                fs.writeFileSync(note, floatText + result.text + "\n");
                found.push({ policy, seed, tick: result.tick, why: result.why, note });
                fs.rmSync(protoFile, { force: true });
                fs.rmSync(portFile, { force: true });
            }
        }
    } finally {
        await session.browser.close();
    }
    found.sort((a, b) => a.tick - b.tick || a.seed - b.seed);
    const report = path.join(work, "diffs.txt");
    const lines = found.length
        ? found.map((item) => `${item.policy} seed ${item.seed} tick ${item.tick} ${item.why}`)
        : ["no mismatches"];
    fs.writeFileSync(report, lines.join("\n") + "\n");
    console.log(lines.join("\n"));
    if (found.length) process.exitCode = 1;
}

main().catch((error) => {
    console.error(error);
    process.exit(1);
});
