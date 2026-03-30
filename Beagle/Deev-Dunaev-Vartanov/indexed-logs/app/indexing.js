function uniq(values) {
  return Array.from(new Set(values));
}

function sortIds(ids) {
  return uniq(ids).sort();
}

function splitIntoTokens(value) {
  return String(value || "")
    .toLowerCase()
    .split(/[^\p{L}\p{N}_-]+/u)
    .map((token) => normalizeToken(token))
    .filter(Boolean);
}

function normalizeToken(token) {
  return String(token || "")
    .toLowerCase()
    .replace(/[^\p{L}\p{N}_-]+/gu, "")
    .trim();
}

function tokenizeLog(log) {
  const pieces = [log.message, log.source, log.level, ...(log.tags || [])]
    .filter(Boolean)
    .join(" ");

  return sortIds(splitIntoTokens(pieces));
}

function tokenizeSearchTerm(term) {
  return sortIds(splitIntoTokens(term));
}

function toDayKey(timestamp) {
  return new Date(timestamp).toISOString().slice(0, 10);
}

function addToIndex(index, key, id) {
  if (!key) {
    return;
  }

  if (!index[key]) {
    index[key] = [];
  }

  index[key].push(id);
}

function finalizeIndex(index) {
  const output = {};

  for (const [key, ids] of Object.entries(index)) {
    output[key] = sortIds(ids);
  }

  return output;
}

function buildIndexEntries(logs) {
  const byDay = {};
  const byLevel = {};
  const bySource = {};
  const byTerm = {};

  for (const log of logs) {
    addToIndex(byDay, toDayKey(log.timestamp), log.id);
    addToIndex(byLevel, log.level, log.id);
    addToIndex(bySource, log.source, log.id);

    for (const token of tokenizeLog(log)) {
      addToIndex(byTerm, token, log.id);
    }
  }

  return {
    byDay: finalizeIndex(byDay),
    byLevel: finalizeIndex(byLevel),
    bySource: finalizeIndex(bySource),
    byTerm: finalizeIndex(byTerm),
  };
}

function buildIndexManifest(logs, entries) {
  return {
    generatedAt: new Date().toISOString(),
    logCount: logs.length,
    buckets: {
      byDay: Object.keys(entries.byDay).length,
      byLevel: Object.keys(entries.byLevel).length,
      bySource: Object.keys(entries.bySource).length,
      byTerm: Object.keys(entries.byTerm).length,
    },
  };
}

export {
  buildIndexManifest,
  buildIndexEntries,
  normalizeToken,
  toDayKey,
  tokenizeLog,
  tokenizeSearchTerm,
};
