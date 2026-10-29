import fs from "fs";
import path from "path";
import { spawn } from "node:child_process";
import { fileURLToPath } from "url";
import { openPrototype } from "./drive.mjs";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../..");
const work = path.join(here, "work");

function writeScript(file, shotAt, image) {
    const lines = [
        "0.02 resize 1600 1000",
        "0.08 press Space",
        "0.20 press 3",
        shotAt.toFixed(2) + " shot " + image,
        (shotAt + 0.4).toFixed(2) + " quit",
        "",
    ];
    fs.writeFileSync(file, lines.join("\n"));
}

function runPort(tick, shotAt, image) {
    const script = path.join(work, "freeze-" + tick + ".txt");
    writeScript(script, shotAt, image);
    const exe = path.join(root, "build", "dev", "tieqi.exe");
    const replay = path.join(work, "seed-1.json");
    return new Promise((resolve, reject) => {
        const child = spawn(exe, ["--replay", replay, "--seed", "1", "--freeze-at", String(tick), "--script", script], {
            cwd: root,
            stdio: "inherit",
        });
        child.on("error", reject);
        child.on("exit", (code) => {
            if (code === 0) {
                resolve();
            } else {
                reject(new Error("tieqi exited " + code + " at tick " + tick));
            }
        });
    });
}

async function shotPrototype(page, tick, image, banner) {
    await page.evaluate(() => window.__boot({ seed: 1, policy: "scripted", noAI: false }));
    await page.evaluate(
        async (shot) => {
            const rawStep = window.__step;
            window.__step = function (dt) {
                const now = window.__S().tick || 0;
                if (now + 1 === shot.limit) {
                    window.__S().fx = [];
                }
                return rawStep(dt);
            };
            await window.__run(shot.limit, false);
            window.__step = rawStep;
            window.__f.resize();
            window.__f.draw(0);
            for (const overlay of document.querySelectorAll(".ov")) {
                overlay.hidden = true;
            }
            const plate = document.querySelector(".banner");
            if (plate) {
                plate.style.animation = "none";
                plate.style.opacity = shot.banner ? "1" : "0";
            }
        },
        { limit: tick, banner },
    );
    await page.locator("#cv").screenshot({ path: image });
}

async function main() {
    const only = process.argv.includes("--proto") ? "proto" : process.argv.includes("--port") ? "port" : "both";
    fs.mkdirSync(work, { recursive: true });
    const shots = [
        { name: "impact", tick: 59, at: 4.2, banner: false },
        { name: "disperse", tick: 255, at: 14.2, banner: false },
        { name: "merge", tick: 310, at: 17.2, banner: false },
        { name: "phase", tick: 1800, at: 94.0, banner: true },
    ];
    if (only !== "port") {
        const session = await openPrototype("tick");
        try {
            await session.page.setViewportSize({ width: 1600, height: 1000 });
            for (const shot of shots) {
                const image = path.join(work, "proto-" + shot.name + ".png");
                await shotPrototype(session.page, shot.tick, image, shot.banner);
                console.log("prototype", shot.name, shot.tick);
            }
        } finally {
            await session.browser.close();
        }
    }
    if (only === "proto") {
        return;
    }
    for (const shot of shots) {
        const image = path.join(work, "port-" + shot.name + ".png");
        await runPort(shot.tick, shot.at, image);
        console.log("port", shot.name, shot.tick);
    }
}

main().catch((error) => {
    console.error(error);
    process.exit(1);
});
