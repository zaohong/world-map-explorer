'use strict';

const fs = require('node:fs');
const path = require('node:path');

const expected = {
  1000: { Beijing: 'Liao Dynasty', Paris: 'Kingdom of France', London: 'Kingdom of England', Cairo: 'Fatimid Caliphate', Angkor: 'Khmer Empire', Tokyo: 'Heian Japan' },
  1200: { Beijing: 'Great Jin', Paris: 'Kingdom of France', London: 'Angevin Empire', Cairo: 'Ayyubid Sultanate', Angkor: 'Khmer Empire', Tokyo: 'Kamakura Shogunate' },
  1400: { Beijing: 'Ming Dynasty', Paris: 'Kingdom of France', London: 'Kingdom of England', Cairo: 'Mamluk Sultanate', Angkor: 'Khmer Empire', Tokyo: 'Ashikaga Shogunate' },
  1600: { Beijing: 'Ming Dynasty', Paris: 'Kingdom of France', London: 'Kingdom of England', Cairo: 'Ottoman Empire', Angkor: 'Khmer Empire', Tokyo: 'Tokugawa Shogunate' },
  1800: { Beijing: 'Qing Dynasty', Paris: 'French Consulate', London: 'Kingdom of Great Britain', Cairo: 'Ottoman Empire', Angkor: 'Rattanakosin Kingdom', Tokyo: 'Tokugawa Shogunate' }
};
const points = {
  Beijing: [39.9, 116.4], Paris: [48.85, 2.35], London: [51.5, -0.12],
  Cairo: [30.04, 31.24], Angkor: [13.4, 103.86], Tokyo: [35.7, 139.7]
};

let checks = 0;
let failures = 0;
function check(condition, label) {
  checks++;
  process.stdout.write(`${condition ? 'PASS' : 'FAIL'}: ${label}\n`);
  if (!condition) failures++;
}
function cellAt(module, latitude, longitude) {
  const row = Math.min(559, Math.max(0, Math.floor((90 - latitude) / 180 * 560)));
  const column = Math.min(1119, Math.max(0, Math.floor((longitude + 180) / 360 * 1120)));
  const id = module.cells[row * 1120 + column];
  return id ? module.entities[id - 1] : null;
}

const directory = process.argv[2];
if (!directory) throw new Error('Usage: historical_module_checks.js <release-data-directory>');
const years = fs.readdirSync(directory)
  .map(name => /^world-(-?\d+)\.json$/.exec(name))
  .filter(match => match && Number(match[1]) !== 2026)
  .map(match => Number(match[1]))
  .sort((a, b) => a - b);
check(years.length > 165 && years.length <= 1000 && new Set(years).size === years.length && years[0] === -3400 && years.at(-1) === 2024,
  `${years.length} unique historical year modules stay within the 1,000-node limit and span 3400 BCE through 2024 CE`);
for (const year of [1, 476, 1000, 1066, 1200, 1400, 1492, 1600, 1776, 1800, 1914, 2024]) {
  check(years.includes(year), `timeline retains the established milestone ${year}`);
}
for (const year of years) {
  const file = path.join(directory, `world-${year}.json`);
  const module = JSON.parse(fs.readFileSync(file, 'utf8'));
  const prefix = `world-${year}`;
  check(module.schemaVersion === 1 && module.moduleId === prefix && module.year === year, `${prefix} identity and schema`);
  check(module.gridRows === 560 && module.gridColumns === 1120 && module.cells.length === 627200, `${prefix} covers all 627,200 fine-grid cells`);
  check(module.source?.license === 'CC BY 4.0' && module.source?.attribution?.includes('Bennett et al.') && module.source?.citation === 'https://doi.org/10.1038/s41597-025-04516-9' && module.source?.processedCommit === '05489efba5a5c188a95d00b7678fbf00a028c27f', `${prefix} retains source attribution and pinned snapshot`);
  check(module.entities.length > 0 && module.cells.some(id => id === 0) && module.cells.some(id => id > 0), `${prefix} has polity records and transparent unrecorded cells`);
  check(module.entities.every(entity => Number.isFinite(entity.fromYear) && Number.isFinite(entity.toYear) && entity.fromYear <= year && entity.toYear >= year), `${prefix} source time spans include the selected year`);
  check(module.cells.every(id => Number.isInteger(id) && id >= 0 && id <= module.entities.length), `${prefix} cell references are valid`);
  let sampledCoverage = 0;
  for (const [city, coordinates] of Object.entries(points)) {
    const polity = cellAt(module, coordinates[0], coordinates[1]);
    if (polity) sampledCoverage++;
    if (expected[year]) check(polity?.name === expected[year][city], `${prefix} sampled ${city}: ${expected[year][city]}`);
  }
  check(expected[year] ? sampledCoverage >= 3 : true, `${prefix} has known-city samples where reference data is available (${sampledCoverage}/6)`);
}
process.stdout.write(`TOTAL: ${checks} passed, ${failures} failed (${checks} checks)\n`);
process.exitCode = failures ? 1 : 0;
