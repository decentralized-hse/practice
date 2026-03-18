import {
  buildIndexEntries,
  buildIndexManifest,
  toDayKey,
  tokenizeLog,
  tokenizeSearchTerm,
} from "./indexing.js";

const DEFAULT_BASE_URL = "";
const LOGS_DIR = "/logs";
const INDEXES_DIR = "/indexes";
const MARKER_SUFFIX = ".txt";

function resolveBaseUrl() {
  if (window.__BEAGLE_BASE_URL__) {
    return String(window.__BEAGLE_BASE_URL__).replace(/\/+$/, "");
  }

  const fromQuery = new URLSearchParams(window.location.search).get("base");
  if (fromQuery) {
    return fromQuery.replace(/\/+$/, "");
  }

  return DEFAULT_BASE_URL;
}

const BASE_URL = resolveBaseUrl();

function buildUrl(path) {
  const normalizedPath = path.startsWith("/") ? path : `/${path}`;
  return `${BASE_URL}${normalizedPath}`;
}

function normalizeDirectoryPath(path) {
  const normalized = path.startsWith("/") ? path : `/${path}`;
  return normalized.endsWith("/") ? normalized : `${normalized}/`;
}

function ensureOk(response, path) {
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText} for ${path}`);
  }

  return response;
}

async function readJsonOrNull(path) {
  const response = await fetch(buildUrl(path));
  if (response.status === 404) {
    return null;
  }

  ensureOk(response, path);
  return response.json();
}

function parseDirectoryListing(text) {
  const trimmed = text.trim();
  if (!trimmed) {
    return [];
  }

  try {
    const json = JSON.parse(trimmed);
    if (Array.isArray(json)) {
      return json
        .map((value) => (typeof value === "string" ? value : value?.name))
        .filter(Boolean);
    }

    if (Array.isArray(json.entries)) {
      return json.entries
        .map((value) => (typeof value === "string" ? value : value?.name))
        .filter(Boolean);
    }
  } catch {
    // Plain-text listing is also supported.
  }

  return trimmed
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter(Boolean)
    .map((line) => line.replace(/\/$/, ""))
    .filter((line) => line !== "." && line !== "..");
}

function uniq(values) {
  return Array.from(new Set(values));
}

async function listDir(path) {
  const directoryPath = normalizeDirectoryPath(path);
  const response = await fetch(buildUrl(directoryPath));
  ensureOk(response, directoryPath);
  const text = await response.text();
  return parseDirectoryListing(text);
}

async function getJson(path) {
  const response = await fetch(buildUrl(path));
  ensureOk(response, path);
  return response.json();
}

async function getTextOrNull(path) {
  const response = await fetch(buildUrl(path));
  if (response.status === 404) {
    return null;
  }

  ensureOk(response, path);
  return response.text();
}

async function postRaw(path, rawText, contentType = "text/plain; charset=utf-8") {
  const response = await fetch(buildUrl(path), {
    method: "POST",
    headers: {
      "Content-Type": contentType,
    },
    body: rawText,
  });

  ensureOk(response, path);
  return response;
}

async function postJson(path, data) {
  return postRaw(path, JSON.stringify(data, null, 2), "application/json; charset=utf-8");
}

async function readIndexFile(path) {
  const text = await getTextOrNull(path);
  if (!text) {
    return { ids: [] };
  }

  let data;
  try {
    data = JSON.parse(text);
  } catch {
    return { ids: [] };
  }

  if (!Array.isArray(data.ids)) {
    return { ids: [] };
  }

  return { ids: uniq(data.ids) };
}

async function writeIndexFile(path, ids) {
  await postJson(path, { ids: uniq(ids).sort() });
}

async function writeIndexGroup(groupName, entries) {
  const dirPath = `${INDEXES_DIR}/${groupName}`;
  let existingFiles = [];

  try {
    existingFiles = await listDir(dirPath);
  } catch {
    existingFiles = [];
  }

  const nextFiles = new Set(Object.keys(entries).map((key) => `${key}.json`));

  for (const [key, ids] of Object.entries(entries)) {
    await writeIndexFile(`${dirPath}/${encodeURIComponent(key)}.json`, ids);
  }

  for (const staleFile of existingFiles) {
    if (!staleFile.endsWith(".json") || nextFiles.has(staleFile)) {
      continue;
    }

    await writeIndexFile(`${dirPath}/${staleFile}`, []);
  }
}

async function writeManifest(logs, entries) {
  const manifest = buildIndexManifest(logs, entries);
  await postJson(`${INDEXES_DIR}/manifest.json`, manifest);
  return manifest;
}

async function updateIndexAdd(path, id) {
  const current = await readIndexFile(path);
  if (!current.ids.includes(id)) {
    current.ids.push(id);
    await writeIndexFile(path, current.ids);
  }
}

async function updateIndexRemove(path, id) {
  const current = await readIndexFile(path);
  const nextIds = current.ids.filter((entry) => entry !== id);
  await writeIndexFile(path, nextIds);
}

function markerFileName(bucketKey, id) {
  return `${encodeURIComponent(bucketKey)}__${encodeURIComponent(id)}${MARKER_SUFFIX}`;
}

function parseMarkerId(fileName) {
  if (!fileName.endsWith(MARKER_SUFFIX)) {
    return null;
  }

  const separatorIndex = fileName.indexOf("__");
  if (separatorIndex === -1) {
    return null;
  }

  const encodedId = fileName.slice(separatorIndex + 2, -MARKER_SUFFIX.length);
  return decodeURIComponent(encodedId);
}

async function writeMarkerFile(groupName, bucketKey, id) {
  const markerPath = `${INDEXES_DIR}/${groupName}/${markerFileName(bucketKey, id)}`;
  await postRaw(markerPath, `${id}\n`);
}

async function listMarkerIds(groupName, bucketKey) {
  const prefix = `${encodeURIComponent(bucketKey)}__`;

  try {
    const entries = await listDir(`${INDEXES_DIR}/${groupName}`);
    return uniq(
      entries
        .filter((entry) => entry.startsWith(prefix) && entry.endsWith(MARKER_SUFFIX))
        .map(parseMarkerId)
        .filter(Boolean),
    );
  } catch {
    return [];
  }
}

async function readBucketIds(groupName, bucketKey) {
  const markerIds = await listMarkerIds(groupName, bucketKey);
  const legacyIndex = await readIndexFile(
    `${INDEXES_DIR}/${groupName}/${encodeURIComponent(bucketKey)}.json`,
  );

  return uniq([...markerIds, ...legacyIndex.ids]);
}

function levelIndexPath(level) {
  return `${INDEXES_DIR}/by-level/${encodeURIComponent(level)}.json`;
}

function sourceIndexPath(source) {
  return `${INDEXES_DIR}/by-source/${encodeURIComponent(source)}.json`;
}

function dayIndexPath(day) {
  return `${INDEXES_DIR}/by-day/${encodeURIComponent(day)}.json`;
}

function termIndexPath(term) {
  return `${INDEXES_DIR}/by-term/${encodeURIComponent(term)}.json`;
}

function logPath(id) {
  return `${LOGS_DIR}/${id}.json`;
}

async function getAllLogIds() {
  const entries = await listDir(LOGS_DIR);
  return entries
    .filter((entry) => entry.endsWith(".json"))
    .map((entry) => entry.replace(/\.json$/, ""));
}

async function getLogById(id) {
  return getJson(logPath(id));
}

async function getLogsByIds(ids) {
  const logs = await Promise.all(
    uniq(ids).map(async (id) => {
      try {
        return await getLogById(id);
      } catch {
        return null;
      }
    }),
  );

  return logs.filter(Boolean).sort((a, b) => b.timestamp.localeCompare(a.timestamp));
}

async function getAllLogs() {
  const ids = await getAllLogIds();
  return getLogsByIds(ids);
}

async function rebuildIndexes() {
  const logs = await getAllLogs();
  const entries = buildIndexEntries(logs);
  const manifest = await writeManifest(logs, entries);

  for (const [day, ids] of Object.entries(entries.byDay)) {
    for (const id of ids) {
      await writeMarkerFile("by-day", day, id);
    }
  }

  for (const [level, ids] of Object.entries(entries.byLevel)) {
    for (const id of ids) {
      await writeMarkerFile("by-level", level, id);
    }
  }

  for (const [source, ids] of Object.entries(entries.bySource)) {
    for (const id of ids) {
      await writeMarkerFile("by-source", source, id);
    }
  }

  for (const [term, ids] of Object.entries(entries.byTerm)) {
    for (const id of ids) {
      await writeMarkerFile("by-term", term, id);
    }
  }

  return {
    logCount: manifest.logCount,
    termBucketCount: manifest.buckets.byTerm,
  };
}

async function addLogToIndexes(log) {
  await Promise.all([
    writeMarkerFile("by-level", log.level, log.id),
    writeMarkerFile("by-source", log.source, log.id),
    writeMarkerFile("by-day", toDayKey(log.timestamp), log.id),
    ...tokenizeLog(log).map((token) => writeMarkerFile("by-term", token, log.id)),
  ]);
}

async function removeLogFromIndexes(log) {
  await Promise.all([
    updateIndexRemove(levelIndexPath(log.level), log.id),
    updateIndexRemove(sourceIndexPath(log.source), log.id),
    updateIndexRemove(dayIndexPath(toDayKey(log.timestamp)), log.id),
    ...tokenizeLog(log).map((token) => updateIndexRemove(termIndexPath(token), log.id)),
  ]);
}

async function saveLog(log) {
  const previous = await readJsonOrNull(logPath(log.id));

  if (previous) {
    await removeLogFromIndexes(previous);
  }

  await postJson(logPath(log.id), log);
  await addLogToIndexes(log);
  return log;
}

function intersectIdSets(sets) {
  const nonEmptySets = sets.filter(Boolean);
  if (nonEmptySets.length === 0) {
    return null;
  }

  const sortedSets = [...nonEmptySets].sort((left, right) => left.size - right.size);
  if (sortedSets[0].size === 0) {
    return [];
  }

  const [first, ...rest] = sortedSets;
  return Array.from(first).filter((id) => rest.every((set) => set.has(id)));
}

function matchesAllTokens(log, tokens) {
  if (tokens.length === 0) {
    return true;
  }

  const haystack = [log.message, log.source, log.level, ...(log.tags || [])]
    .filter(Boolean)
    .join(" ")
    .toLowerCase();

  return tokens.every((token) => haystack.includes(token));
}

function matchesStructuredFilters(log, { level, source, date }) {
  if (level && log.level !== level) {
    return false;
  }

  if (source && log.source !== source) {
    return false;
  }

  if (date && toDayKey(log.timestamp) !== date) {
    return false;
  }

  return true;
}

async function searchLogs({ term = "", level = "", source = "", date = "" }) {
  const filters = [];

  if (level) {
    filters.push(new Set(await readBucketIds("by-level", level)));
  }

  if (source) {
    filters.push(new Set(await readBucketIds("by-source", source)));
  }

  if (date) {
    filters.push(new Set(await readBucketIds("by-day", date)));
  }

  const termTokens = tokenizeSearchTerm(term);

  for (const token of termTokens) {
    const ids = await readBucketIds("by-term", token);
    if (ids.length === 0) {
      return [];
    }

    filters.push(new Set(ids));
  }

  const candidateIds = intersectIdSets(filters);
  const logs = candidateIds === null ? await getAllLogs() : await getLogsByIds(candidateIds);

  return logs.filter(
    (log) =>
      matchesStructuredFilters(log, { level, source, date }) &&
      matchesAllTokens(log, termTokens),
  );
}

export {
  BASE_URL,
  getAllLogIds,
  getAllLogs,
  getJson,
  getLogById,
  getLogsByIds,
  listDir,
  postJson,
  rebuildIndexes,
  saveLog,
  searchLogs,
};
