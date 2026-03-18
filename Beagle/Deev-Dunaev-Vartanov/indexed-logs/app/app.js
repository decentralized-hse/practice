import { getAllLogs, rebuildIndexes, saveLog, searchLogs } from "./api.js";

const createForm = document.querySelector("#create-log-form");
const searchForm = document.querySelector("#search-form");
const refreshButton = document.querySelector("#refresh-button");
const rebuildIndexesButton = document.querySelector("#rebuild-indexes-button");
const resetSearchButton = document.querySelector("#reset-search-button");
const logsContainer = document.querySelector("#logs-container");
const statusText = document.querySelector("#status-text");
const logCardTemplate = document.querySelector("#log-card-template");

function generateLogId() {
  if (globalThis.crypto?.randomUUID) {
    return `log-${crypto.randomUUID()}`;
  }

  return `log-${Date.now()}-${Math.random().toString(16).slice(2, 10)}`;
}

function parseTags(value) {
  return value
    .split(",")
    .map((tag) => tag.trim())
    .filter(Boolean);
}

function toIsoTimestamp(value) {
  if (!value) {
    return new Date().toISOString();
  }

  return new Date(value).toISOString();
}

function formatTimestamp(timestamp) {
  return new Date(timestamp).toLocaleString();
}

function getSearchParamsFromForm() {
  const formData = new FormData(searchForm);
  return {
    term: String(formData.get("term") || "").trim(),
    level: String(formData.get("level") || "").trim(),
    source: String(formData.get("source") || "").trim(),
    date: String(formData.get("date") || "").trim(),
  };
}

function getSearchParamsFromUrl() {
  const params = new URLSearchParams(window.location.search);
  return {
    term: params.get("term")?.trim() || "",
    level: params.get("level")?.trim() || "",
    source: params.get("source")?.trim() || "",
    date: params.get("date")?.trim() || "",
  };
}

function hasActiveSearch(params) {
  return Object.values(params).some(Boolean);
}

function applySearchParamsToForm(params) {
  document.querySelector("#term-filter").value = params.term || "";
  document.querySelector("#level-filter").value = params.level || "";
  document.querySelector("#source-filter").value = params.source || "";
  document.querySelector("#date-filter").value = params.date || "";
}

function syncSearchUrl(params) {
  const url = new URL(window.location.href);

  for (const [key, value] of Object.entries(params)) {
    if (value) {
      url.searchParams.set(key, value);
    } else {
      url.searchParams.delete(key);
    }
  }

  window.history.replaceState({}, "", url);
}

function setStatus(message, isError = false) {
  statusText.textContent = message;
  statusText.style.color = isError ? "var(--error)" : "var(--muted)";
}

function renderEmptyState(message) {
  logsContainer.innerHTML = "";
  const empty = document.createElement("div");
  empty.className = "empty-state";
  empty.textContent = message;
  logsContainer.append(empty);
}

function renderLogs(logs) {
  logsContainer.innerHTML = "";

  if (logs.length === 0) {
    renderEmptyState("No logs found yet.");
    return;
  }

  const fragment = document.createDocumentFragment();

  for (const log of logs) {
    const node = logCardTemplate.content.cloneNode(true);
    const card = node.querySelector(".log-card");
    const level = node.querySelector(".log-level");
    const source = node.querySelector(".log-source");
    const message = node.querySelector(".log-message");
    const time = node.querySelector(".log-time");
    const tags = node.querySelector(".log-tags");
    level.textContent = log.level;
    level.dataset.level = log.level;
    source.textContent = log.source;
    message.textContent = log.message;
    time.dateTime = log.timestamp;
    time.textContent = formatTimestamp(log.timestamp);

    for (const tag of log.tags || []) {
      const pill = document.createElement("span");
      pill.className = "log-tag";
      pill.textContent = `#${tag}`;
      tags.append(pill);
    }

    card.dataset.id = log.id;
    fragment.append(node);
  }

  logsContainer.append(fragment);
}

async function refreshAllLogs() {
  setStatus("Loading logs...");

  try {
    const logs = await getAllLogs();
    renderLogs(logs);
    setStatus(`Loaded ${logs.length} log(s).`);
  } catch (error) {
    console.error(error);
    renderEmptyState("Failed to load logs.");
    setStatus(`Load failed: ${error.message}`, true);
  }
}

async function handleCreateLog(event) {
  event.preventDefault();

  const formData = new FormData(createForm);
  const log = {
    id: generateLogId(),
    timestamp: toIsoTimestamp(String(formData.get("timestamp") || "")),
    source: String(formData.get("source") || "").trim(),
    level: String(formData.get("level") || "INFO").trim().toUpperCase(),
    message: String(formData.get("message") || "").trim(),
    tags: parseTags(String(formData.get("tags") || "")),
  };

  setStatus(`Saving ${log.id}...`);

  try {
    await saveLog(log);
    createForm.reset();
    document.querySelector("#level-input").value = "INFO";
    setStatus(`Saved ${log.id}.`);
    await refreshAllLogs();
  } catch (error) {
    console.error(error);
    setStatus(`Save failed: ${error.message}`, true);
  }
}

async function runSearch(searchParams) {
  setStatus("Searching...");

  try {
    const logs = await searchLogs(searchParams);
    renderLogs(logs);
    setStatus(`Found ${logs.length} log(s).`);
  } catch (error) {
    console.error(error);
    setStatus(`Search failed: ${error.message}`, true);
  }
}

async function handleSearch(event) {
  event.preventDefault();

  const searchParams = getSearchParamsFromForm();
  syncSearchUrl(searchParams);
  await runSearch(searchParams);
}

async function handleRebuildIndexes() {
  setStatus("Rebuilding indexes from all logs...");

  try {
    const result = await rebuildIndexes();
    setStatus(
      `Rebuilt indexes for ${result.logCount} log(s), ${result.termBucketCount} term bucket(s).`,
    );
    await refreshAllLogs();
  } catch (error) {
    console.error(error);
    setStatus(`Index rebuild failed: ${error.message}`, true);
  }
}

function resetSearch() {
  searchForm.reset();
  syncSearchUrl({ term: "", level: "", source: "", date: "" });
  refreshAllLogs();
}

async function initializePage() {
  const initialSearch = getSearchParamsFromUrl();
  applySearchParamsToForm(initialSearch);

  if (hasActiveSearch(initialSearch)) {
    await runSearch(initialSearch);
    return;
  }

  await refreshAllLogs();
}

createForm.addEventListener("submit", handleCreateLog);
searchForm.addEventListener("submit", handleSearch);
refreshButton.addEventListener("click", refreshAllLogs);
rebuildIndexesButton.addEventListener("click", handleRebuildIndexes);
resetSearchButton.addEventListener("click", resetSearch);

initializePage();
