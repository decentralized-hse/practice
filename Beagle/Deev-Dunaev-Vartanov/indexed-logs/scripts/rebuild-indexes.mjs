import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { buildIndexEntries, buildIndexManifest } from "../app/indexing.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, "..");
const logsDir = path.join(rootDir, "logs");
const indexesDir = path.join(rootDir, "indexes");

async function readLogs() {
  const names = (await fs.readdir(logsDir))
    .filter((name) => name.endsWith(".json"))
    .sort();

  const logs = [];
  for (const name of names) {
    const fullPath = path.join(logsDir, name);
    const text = await fs.readFile(fullPath, "utf8");
    logs.push(JSON.parse(text));
  }

  return logs;
}

async function listJsonFiles(dirPath) {
  try {
    return (await fs.readdir(dirPath)).filter((name) => name.endsWith(".json"));
  } catch (error) {
    if (error.code === "ENOENT") {
      return [];
    }

    throw error;
  }
}

async function listMarkerFiles(dirPath) {
  try {
    return (await fs.readdir(dirPath)).filter((name) => name.includes("__") && name.endsWith(".txt"));
  } catch (error) {
    if (error.code === "ENOENT") {
      return [];
    }

    throw error;
  }
}

async function writeIndexGroup(groupName, entries) {
  const dirPath = path.join(indexesDir, groupName);
  await fs.mkdir(dirPath, { recursive: true });

  const existingFiles = await listJsonFiles(dirPath);
  const nextFiles = new Set(Object.keys(entries).map((key) => `${key}.json`));

  for (const [key, ids] of Object.entries(entries)) {
    const filePath = path.join(dirPath, `${key}.json`);
    const payload = JSON.stringify({ ids }, null, 2) + "\n";
    await fs.writeFile(filePath, payload, "utf8");
  }

  for (const staleFile of existingFiles) {
    if (nextFiles.has(staleFile)) {
      continue;
    }

    const stalePath = path.join(dirPath, staleFile);
    await fs.writeFile(stalePath, JSON.stringify({ ids: [] }, null, 2) + "\n", "utf8");
  }
}

function markerFileName(bucketKey, id) {
  return `${encodeURIComponent(bucketKey)}__${encodeURIComponent(id)}.txt`;
}

async function writeMarkerGroup(groupName, entries) {
  const dirPath = path.join(indexesDir, groupName);
  await fs.mkdir(dirPath, { recursive: true });

  const existingMarkers = await listMarkerFiles(dirPath);
  await Promise.all(existingMarkers.map((name) => fs.rm(path.join(dirPath, name), { force: true })));

  for (const [key, ids] of Object.entries(entries)) {
    for (const id of ids) {
      const filePath = path.join(dirPath, markerFileName(key, id));
      await fs.writeFile(filePath, `${id}\n`, "utf8");
    }
  }
}

async function writeManifest(logs, entries) {
  const manifestPath = path.join(indexesDir, "manifest.json");
  const manifest = buildIndexManifest(logs, entries);

  await fs.writeFile(manifestPath, JSON.stringify(manifest, null, 2) + "\n", "utf8");
}

async function main() {
  const logs = await readLogs();
  const entries = buildIndexEntries(logs);

  await writeIndexGroup("by-day", entries.byDay);
  await writeIndexGroup("by-level", entries.byLevel);
  await writeIndexGroup("by-source", entries.bySource);
  await writeIndexGroup("by-term", entries.byTerm);
  await writeMarkerGroup("by-day", entries.byDay);
  await writeMarkerGroup("by-level", entries.byLevel);
  await writeMarkerGroup("by-source", entries.bySource);
  await writeMarkerGroup("by-term", entries.byTerm);
  await writeManifest(logs, entries);

  console.log(
    `Rebuilt indexes for ${logs.length} logs: ` +
      `${Object.keys(entries.byTerm).length} term buckets`,
  );
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
