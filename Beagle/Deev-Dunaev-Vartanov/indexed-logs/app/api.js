const DEFAULT_BASE_URL = "";
const LOGS_DIR = "/logs";
const INDEXES_DIR = "/indexes";

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

function toDayKey(timestamp) {
  return new Date(timestamp).toISOString().slice(0, 10);
}

function normalizeToken(token) {
  return token.toLowerCase().replace(/[^a-z0-9_-]+/g, "").trim();
}

function tokenizeLog(log) {
  const pieces = [log.message, log.source, log.level, ...(log.tags || [])]
    .filter(Boolean)
    .join(" ")
    .toLowerCase()
    .split(/[^a-z0-9_-]+/);

  return uniq(pieces.map(normalizeToken).filter(Boolean));
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
  const data = await readJsonOrNull(path);
  if (!data || !Array.isArray(data.ids)) {
    return { ids: [] };
  }

  return { ids: uniq(data.ids) };
}

async function writeIndexFile(path, ids) {
  await postJson(path, { ids: uniq(ids).sort() });
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

async function addLogToIndexes(log) {
  await Promise.all([
    updateIndexAdd(levelIndexPath(log.level), log.id),
    updateIndexAdd(sourceIndexPath(log.source), log.id),
    updateIndexAdd(dayIndexPath(toDayKey(log.timestamp)), log.id),
    ...tokenizeLog(log).map((token) => updateIndexAdd(termIndexPath(token), log.id)),
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

  const [first, ...rest] = nonEmptySets;
  return Array.from(first).filter((id) => rest.every((set) => set.has(id)));
}

function matchesTerm(log, term) {
  const haystack = [log.message, log.source, log.level, ...(log.tags || [])]
    .filter(Boolean)
    .join(" ")
    .toLowerCase();

  return haystack.includes(term.toLowerCase());
}

async function searchLogs({ term = "", level = "", source = "", date = "" }) {
  const filters = [];

  if (level) {
    const levelIndex = await readIndexFile(levelIndexPath(level));
    filters.push(new Set(levelIndex.ids));
  }

  if (source) {
    const sourceIndex = await readIndexFile(sourceIndexPath(source));
    filters.push(new Set(sourceIndex.ids));
  }

  if (date) {
    const dayIndex = await readIndexFile(dayIndexPath(date));
    filters.push(new Set(dayIndex.ids));
  }

  const termTokens = uniq(
    term
      .toLowerCase()
      .split(/[^a-z0-9_-]+/)
      .map(normalizeToken)
      .filter(Boolean),
  );

  for (const token of termTokens) {
    const index = await readIndexFile(termIndexPath(token));
    if (index.ids.length > 0) {
      filters.push(new Set(index.ids));
    }
  }

  const candidateIds = intersectIdSets(filters);
  const logs = candidateIds === null ? await getAllLogs() : await getLogsByIds(candidateIds);

  if (!term.trim()) {
    return logs;
  }

  return logs.filter((log) => matchesTerm(log, term));
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
  saveLog,
  searchLogs,
};
