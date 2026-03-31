#!/usr/bin/env node
//
// Beagle CI Daemon
//
// Polls be-srv for file changes, runs build/test commands,
// serves results via its own HTTP on CI_PORT.
//
// Usage: node daemon.js [be-srv-url] [project-dir]
//

const http = require("http");
const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const BE_URL = process.argv[2] || "http://127.0.0.1:8800";
const PROJECT = process.argv[3] || "/tmp/testproject";
const POLL_INTERVAL = parseInt(process.env.CI_POLL_MS || "3000", 10);
const CI_PORT = parseInt(process.env.CI_PORT || "8801", 10);
const CI_DIR = path.join(PROJECT, "ci");

let prevFiles = {};
let builds = [];
let buildNo = 0;

// HTTP helpers

function httpGet(urlStr) {
    return new Promise((resolve, reject) => {
        http.get(urlStr, (res) => {
            let data = "";
            res.on("data", (c) => (data += c));
            res.on("end", () => resolve({ status: res.statusCode, body: data }));
        }).on("error", reject);
    });
}

function httpPost(urlStr, body) {
    return new Promise((resolve, reject) => {
        const url = new URL(urlStr);
        const buf = Buffer.from(body, "utf8");
        const opts = {
            hostname: url.hostname,
            port: url.port,
            path: url.pathname,
            method: "POST",
            headers: { "Content-Length": buf.length },
        };
        const req = http.request(opts, (res) => {
            let data = "";
            res.on("data", (c) => (data += c));
            res.on("end", () => resolve({ status: res.statusCode, body: data }));
        });
        req.on("error", reject);
        req.write(buf);
        req.end();
    });
}

// Simple string hash (djb2)

function hash(str) {
    let h = 5381;
    for (let i = 0; i < str.length; i++) {
        h = ((h << 5) + h + str.charCodeAt(i)) & 0xffffffff;
    }
    return h.toString(16);
}

// Fetch file listing from be-srv

async function fetchFileList() {
    const res = await httpGet(BE_URL + "/");
    if (res.status !== 200) return [];
    return res.body
        .split("\n")
        .map((l) => l.trim())
        .filter((l) => l && !l.startsWith("ci/") && l !== ".be"
            && l !== "ci.json" && l !== "index.html" && l !== "builds.json");
}

// Fetch file content

async function fetchFile(name) {
    const res = await httpGet(BE_URL + "/" + encodeURIComponent(name));
    if (res.status !== 200) return null;
    return res.body;
}

// Fetch CI config from be-srv

async function fetchConfig() {
    const defaults = {
        name: "default",
        build: "echo 'no ci.json found'",
        test: "echo 'no tests'",
    };
    try {
        const res = await httpGet(BE_URL + "/ci.json");
        if (res.status === 200) {
            return { ...defaults, ...JSON.parse(res.body) };
        }
    } catch {}
    return defaults;
}

// Detect changes

async function detectChanges() {
    const files = await fetchFileList();
    const currFiles = {};
    const changed = [];

    for (const f of files) {
        const content = await fetchFile(f);
        if (content === null) continue;
        const h = hash(content);
        currFiles[f] = h;
        if (prevFiles[f] !== h) {
            changed.push(f);
        }
    }

    for (const f of Object.keys(prevFiles)) {
        if (!(f in currFiles)) changed.push(f);
    }

    prevFiles = currFiles;
    return changed;
}

// Run a shell command

function runCommand(cmd, cwd) {
    const start = Date.now();
    try {
        const out = execSync(cmd, {
            cwd,
            timeout: 120000,
            encoding: "utf8",
            stdio: ["pipe", "pipe", "pipe"],
        });
        return { ok: true, ms: Date.now() - start, log: out };
    } catch (e) {
        return {
            ok: false,
            ms: Date.now() - start,
            log: (e.stdout || "") + "\n" + (e.stderr || ""),
            code: e.status,
        };
    }
}

// Run build

async function runBuild(changedFiles) {
    const config = await fetchConfig();
    buildNo++;
    const ts = new Date().toISOString();

    console.log(`\n=== Build #${buildNo} at ${ts} ===`);
    console.log(`  changed: ${changedFiles.join(", ")}`);

    // build step
    console.log(`  build: ${config.build}`);
    const buildResult = runCommand(config.build, PROJECT);
    console.log(`  build ${buildResult.ok ? "OK" : "FAIL"} (${buildResult.ms}ms)`);

    // test step (only if build passed)
    let testResult = { ok: false, ms: 0, log: "skipped (build failed)" };
    if (buildResult.ok) {
        console.log(`  test: ${config.test}`);
        testResult = runCommand(config.test, PROJECT);
        console.log(`  test ${testResult.ok ? "OK" : "FAIL"} (${testResult.ms}ms)`);
    }

    const result = {
        id: buildNo,
        ts,
        files: changedFiles,
        project: config.name,
        build: {
            cmd: config.build,
            ok: buildResult.ok,
            ms: buildResult.ms,
            log: buildResult.log.slice(-4096),
        },
        test: {
            cmd: config.test,
            ok: testResult.ok,
            ms: testResult.ms,
            log: testResult.log.slice(-4096),
        },
        status: buildResult.ok && testResult.ok ? "pass" : "fail",
    };

    builds.push(result);
    if (builds.length > 50) builds = builds.slice(-50);

    const json = JSON.stringify(builds, null, 2);

    // persist to disk (fallback)
    fs.mkdirSync(CI_DIR, { recursive: true });
    fs.writeFileSync(path.join(CI_DIR, "builds.json"), json);

    // persist to beagle via HTTP POST
    try {
        const res = await httpPost(BE_URL + "/builds.json", json);
        if (res.status === 200) {
            console.log("  posted results to beagle");
        } else {
            console.error("  beagle POST returned", res.status);
        }
    } catch (e) {
        console.error("  beagle POST failed:", e.message);
    }

    return result;
}

// Poll loop

async function poll() {
    try {
        const changed = await detectChanges();
        if (changed.length > 0) {
            await runBuild(changed);
        }
    } catch (e) {
        console.error("poll error:", e.message);
    }
    setTimeout(poll, POLL_INTERVAL);
}

// CI HTTP server (serves index.html + builds.json)

const INDEX_HTML = fs.readFileSync(
    path.join(__dirname, "index.html"),
    "utf8"
);

const ciServer = http.createServer((req, res) => {
    res.setHeader("Access-Control-Allow-Origin", "*");

    if (req.url === "/" || req.url === "/index.html") {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(INDEX_HTML);
        return;
    }

    if (req.url === "/builds.json") {
        res.writeHead(200, { "Content-Type": "application/json" });
        res.end(JSON.stringify(builds, null, 2));
        return;
    }

    res.writeHead(404);
    res.end("not found");
});

// Start

console.log("Beagle CI Daemon");
console.log(`  be-srv:   ${BE_URL}`);
console.log(`  project:  ${PROJECT}`);
console.log(`  poll:     ${POLL_INTERVAL}ms`);
console.log(`  ci web:   http://0.0.0.0:${CI_PORT}`);

// load previous builds
const bfile = path.join(CI_DIR, "builds.json");
try {
    builds = JSON.parse(fs.readFileSync(bfile, "utf8"));
    buildNo = builds.length > 0 ? builds[builds.length - 1].id : 0;
    console.log(`  loaded ${builds.length} previous builds`);
} catch {}

ciServer.listen(CI_PORT, "0.0.0.0", () => {
    console.log("waiting for changes...\n");
    poll();
});
