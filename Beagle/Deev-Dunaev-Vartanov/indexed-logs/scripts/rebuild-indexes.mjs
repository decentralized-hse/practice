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
