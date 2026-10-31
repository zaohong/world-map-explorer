'use strict';

const fs = require('node:fs');
const path = require('node:path');

const ROWS = 560;
const COLUMNS = 1120;
const CELL_WIDTH = 360 / COLUMNS;
const CELL_HEIGHT = 180 / ROWS;
const HISTORY_ATLAS_COMMIT = '05489efba5a5c188a95d00b7678fbf00a028c27f';
function legacyYears() {
  return [
    ...Array.from({ length: 25 }, (_, index) => -3400 + index * 100),
    ...Array.from({ length: 19 }, (_, index) => -950 + index * 50),
    1,
    ...Array.from({ length: 9 }, (_, index) => (index + 1) * 50),
    ...Array.from({ length: 41 }, (_, index) => 500 + index * 25),
    ...Array.from({ length: 40 }, (_, index) => 1510 + index * 10),
    ...Array.from({ length: 24 }, (_, index) => 1905 + index * 5),
    2024, 476, 1066, 1492, 1776, 1914
  ];
}

function parseBase(file) {
  const source = fs.readFileSync(file, 'utf8');
  const marker = 'Object.assign(window.HA_DATA,';
  const start = source.indexOf(marker);
  const end = source.lastIndexOf(');');
  if (start < 0 || end <= start) throw new Error(`Invalid History Atlas base data: ${file}`);
  const jsonStart = start + marker.length;
  return JSON.parse(source.slice(jsonStart, end));
}

function parseChunk(file) {
  const source = fs.readFileSync(file, 'utf8');
  const start = source.indexOf('{');
  const end = source.lastIndexOf(')');
  if (start < 0 || end <= start) throw new Error(`Invalid History Atlas chunk: ${file}`);
  const topology = JSON.parse(source.slice(start, end));
  if (topology.type !== 'Topology' || !topology.transform || !topology.objects?.u?.geometries) {
    throw new Error(`Unexpected TopoJSON structure: ${file}`);
  }
  topology.decodedArcs = new Map();
  return topology;
}

function arcPoints(topology, reference) {
  const reversed = reference < 0;
  const index = reversed ? ~reference : reference;
  let points = topology.decodedArcs.get(index);
  if (!points) {
    let x = 0, y = 0;
    const [scaleX, scaleY] = topology.transform.scale;
    const [translateX, translateY] = topology.transform.translate;
    points = topology.arcs[index].map(([dx, dy]) => {
      x += dx;
      y += dy;
      return [x * scaleX + translateX, y * scaleY + translateY];
    });
    topology.decodedArcs.set(index, points);
  }
  return reversed ? points.slice().reverse() : points;
}

function joinArcRing(topology, arcReferences) {
  const points = [];
  for (const reference of arcReferences) {
    const arc = arcPoints(topology, reference);
    for (let i = points.length ? 1 : 0; i < arc.length; i++) points.push(arc[i]);
  }
  if (points.length > 1
      && (points[0][0] !== points[points.length - 1][0]
        || points[0][1] !== points[points.length - 1][1])) {
    points.push(points[0]);
  }
  return points;
}

function unwrapRing(source) {
  if (!Array.isArray(source) || source.length < 4) return [];
  const closed = source[0][0] === source[source.length - 1][0]
    && source[0][1] === source[source.length - 1][1];
  const limit = closed ? source.length - 1 : source.length;
  const result = [[source[0][0], source[0][1]]];
  let previous = source[0][0];
  for (let i = 1; i < limit; i++) {
    let longitude = source[i][0];
    longitude += Math.round((previous - longitude) / 360) * 360;
    result.push([longitude, source[i][1]]);
    previous = longitude;
  }
  result.push(result[0].slice());
  return result;
}

function makePolygon(topology, arcRings) {
  if (!Array.isArray(arcRings) || !arcRings.length) return null;
  const outer = unwrapRing(joinArcRing(topology, arcRings[0]));
  if (outer.length < 4) return null;
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  for (const [x, y] of outer) {
    minX = Math.min(minX, x); maxX = Math.max(maxX, x);
    minY = Math.min(minY, y); maxY = Math.max(maxY, y);
  }
  const center = (minX + maxX) / 2;
  const holes = arcRings.slice(1).map(references => {
    const hole = unwrapRing(joinArcRing(topology, references));
    if (hole.length < 4) return [];
    const holeCenter = hole.reduce((sum, point) => sum + point[0], 0) / hole.length;
    const offset = Math.round((center - holeCenter) / 360) * 360;
    return hole.map(([x, y]) => [x + offset, y]);
  }).filter(ring => ring.length >= 4);
  return { outer, holes, minX, minY, maxX, maxY, center };
}

function geometryPolygons(topology, geometry) {
  const source = geometry.type === 'MultiPolygon'
    ? geometry.arcs
    : geometry.type === 'Polygon' ? [geometry.arcs] : [];
  return source.map(polygon => makePolygon(topology, polygon)).filter(Boolean);
}

function insideRing(longitude, latitude, ring) {
  let inside = false;
  for (let i = 0, j = ring.length - 1; i < ring.length; j = i++) {
    const [xi, yi] = ring[i];
    const [xj, yj] = ring[j];
    const crosses = ((yi > latitude) !== (yj > latitude))
      && longitude < (xj - xi) * (latitude - yi) / (yj - yi) + xi;
    if (crosses) inside = !inside;
  }
  return inside;
}

function contains(polygon, longitude, latitude) {
  const x = longitude + Math.round((polygon.center - longitude) / 360) * 360;
  if (!insideRing(x, latitude, polygon.outer)) return false;
  for (const hole of polygon.holes) if (insideRing(x, latitude, hole)) return false;
  return true;
}

function columnsInBox(minimum, maximum) {
  const columns = new Set();
  for (const shift of [-360, 0, 360]) {
    const left = Math.max(-180, minimum - shift);
    const right = Math.min(180, maximum - shift);
    if (left > right) continue;
    const first = Math.max(0, Math.min(COLUMNS - 1, Math.floor((left + 180) / CELL_WIDTH)));
    const last = Math.max(0, Math.min(COLUMNS - 1, Math.floor((right + 180) / CELL_WIDTH)));
    for (let column = first; column <= last; column++) columns.add(column);
  }
  return Array.from(columns);
}

function cellRows(minLatitude, maxLatitude) {
  const first = Math.max(0, Math.min(ROWS - 1, Math.floor((90 - maxLatitude) / CELL_HEIGHT)));
  const last = Math.max(0, Math.min(ROWS - 1, Math.floor((90 - minLatitude) / CELL_HEIGHT)));
  return [first, last];
}

function hslHex(hue, saturation, lightness) {
  const h = ((hue % 360) + 360) % 360;
  const s = Math.max(0, Math.min(100, saturation)) / 100;
  const l = Math.max(0, Math.min(100, lightness)) / 100;
  const chroma = (1 - Math.abs(2 * l - 1)) * s;
  const section = h / 60;
  const second = chroma * (1 - Math.abs(section % 2 - 1));
  let rgb;
  if (section < 1) rgb = [chroma, second, 0];
  else if (section < 2) rgb = [second, chroma, 0];
  else if (section < 3) rgb = [0, chroma, second];
  else if (section < 4) rgb = [0, second, chroma];
  else if (section < 5) rgb = [second, 0, chroma];
  else rgb = [chroma, 0, second];
  const match = l - chroma / 2;
  return '#' + rgb.map(value => Math.round((value + match) * 255).toString(16).padStart(2, '0')).join('').toUpperCase();
}

function colorForEntity(entity, base) {
  const lineageId = entity[1];
  const lineage = base.lins[lineageId];
  const hue = lineage && Number.isFinite(lineage[1]) ? lineage[1] : (entity[0].length * 47) % 360;
  const sourceLightness = Number(entity[3]) || 62;
  const lightness = Math.max(43, Math.min(61, sourceLightness - 11));
  return hslHex(hue, 74, lightness);
}

function chunkIndices(year, base) {
  return base.chunks
    .map(([fromYear, toYear], index) => year >= fromYear && year <= toYear ? index : -1)
    .filter(index => index >= 0);
}

function sourceRows(year, topologies) {
  const rows = [];
  const keys = chunkIndices(year, globalThis.__historyBase).map(index => `c${String(index).padStart(2, '0')}`);
  const seen = new Set();
  let duplicateBoundaryRows = 0;
  for (const key of keys) {
    const topology = topologies.get(key);
    if (!topology) throw new Error(`Missing source chunk ${key}`);
    for (const geometry of topology.objects.u.geometries) {
      const properties = geometry.properties || {};
      if (!Number.isInteger(properties.e)
          || !Number.isFinite(properties.f)
          || !Number.isFinite(properties.t)
          || properties.f > year || properties.t < year) continue;
      const id = `${properties.e}|${properties.f}|${properties.t}`;
      if (seen.has(id)) { duplicateBoundaryRows++; continue; }
      seen.add(id);
      const polygons = geometryPolygons(topology, geometry);
      if (!polygons.length) continue;
      const entity = globalThis.__historyBase.ents[properties.e];
      if (!entity || typeof entity[0] !== 'string' || !entity[0].trim()) continue;
      rows.push({
        entityId: properties.e,
        name: entity[0].trim(),
        wikipedia: entity[4] || '',
        wikidata: entity[5] || '',
        lineageId: Number.isInteger(entity[1]) ? entity[1] : -1,
        fromYear: properties.f,
        toYear: properties.t,
        areaKm2: Number(properties.a) || 0,
        labelPoint: Array.isArray(properties.k) ? properties.k : null,
        polygons,
        color: colorForEntity(entity, globalThis.__historyBase)
      });
    }
  }
  return { rows, duplicateBoundaryRows, chunkKeys: keys };
}

function buildYear(year, base, topologies, outputDirectory) {
  globalThis.__historyBase = base;
  const { rows, duplicateBoundaryRows, chunkKeys } = sourceRows(year, topologies);
  if (!rows.length) throw new Error(`No polity geometries match ${year}`);

  rows.sort((a, b) => b.areaKm2 - a.areaKm2 || a.entityId - b.entityId);
  const entities = rows.map((row, index) => ({
    id: index + 1,
    entityId: row.entityId,
    name: row.name,
    wikipedia: row.wikipedia,
    wikidata: row.wikidata,
    lineageId: row.lineageId,
    fromYear: row.fromYear,
    toYear: row.toYear,
    areaKm2: row.areaKm2,
    labelPoint: row.labelPoint,
    color: row.color
  }));
  const idByEntity = new Map(rows.map((row, index) => [row.entityId, index + 1]));
  const cells = new Uint16Array(ROWS * COLUMNS);
  let assignedCells = 0;
  let overlapReplacements = 0;
  for (const row of rows) {
    const id = idByEntity.get(row.entityId);
    for (const polygon of row.polygons) {
      const [firstRow, lastRow] = cellRows(polygon.minY, polygon.maxY);
      const columns = columnsInBox(polygon.minX, polygon.maxX);
      for (let r = firstRow; r <= lastRow; r++) {
        const latitude = 90 - (r + 0.5) * CELL_HEIGHT;
        for (const c of columns) {
          const longitude = -180 + (c + 0.5) * CELL_WIDTH;
          if (!contains(polygon, longitude, latitude)) continue;
          const offset = r * COLUMNS + c;
          if (cells[offset] && cells[offset] !== id) overlapReplacements++;
          if (!cells[offset]) assignedCells++;
          cells[offset] = id;
        }
      }
    }
  }

  const visible = new Set(cells);
  visible.delete(0);
  const module = {
    schemaVersion: 1,
    moduleId: `world-${year}`,
    year,
    source: {
      name: 'Cliopatria — Seshat Global History Databank',
      attribution: 'Bennett et al. (2025), Cliopatria, Scientific Data 12:247; Seshat Global History Databank. Derived fine-grid assignment based on the pinned History Atlas snapshot.',
      repository: 'https://github.com/Seshat-Global-History-Databank/cliopatria',
      citation: 'https://doi.org/10.1038/s41597-025-04516-9',
      license: 'CC BY 4.0',
      processedSnapshot: 'History Atlas',
      processedRepository: 'https://github.com/laurencefwhite/history-atlas',
      processedCommit: HISTORY_ATLAS_COMMIT
    },
    gridRows: ROWS,
    gridColumns: COLUMNS,
    assignmentRule: 'Assign each 627,200-cell centre to a source polity polygon active in the snapshot year. Larger overlapping polygons are drawn first; the smaller polity replaces them in overlap cells. Unrecorded territory and cells without a source polygon remain transparent.',
    entities,
    cells: Array.from(cells)
  };
  const outputPath = path.join(outputDirectory, `world-${year}.json`);
  fs.writeFileSync(outputPath, JSON.stringify(module));
  const result = {
    year,
    sourceChunks: chunkKeys,
    activePolities: rows.length,
    duplicateBoundaryRows,
    assignedCells,
    overlapReplacements,
    representedPolities: visible.size,
    noGridCentre: rows.length - visible.size,
    bytes: fs.statSync(outputPath).size
  };
  console.log(JSON.stringify(result));
}

const root = path.resolve(__dirname, '..');
const sourceDirectory = path.join(root, 'source-data', 'history-atlas');
const outputDirectory = path.resolve(root, '..', '..', 'WorldMapExplorer-1.21.0', 'data');
fs.mkdirSync(outputDirectory, { recursive: true });
const base = parseBase(path.join(sourceDirectory, 'base.js'));
globalThis.__historyBase = base;
const chunkNumbers = Array.from({ length: base.chunks.length }, (_, index) => index);
const topologies = new Map(chunkNumbers.map(index => {
  const key = `c${String(index).padStart(2, '0')}`;
  return [key, parseChunk(path.join(sourceDirectory, `${key}f.js`))];
}));
const legacy = legacyYears();
if (legacy.length !== 165 || new Set(legacy).size !== 165) throw new Error('Legacy timeline no longer contains 165 unique milestones');
const eventYears = new Set();
let eventReferences = 0;
for (const topology of topologies.values()) {
  for (const geometry of topology.objects.u.geometries) {
    const properties = geometry.properties || {};
    if (!Number.isInteger(properties.e) || !Number.isInteger(properties.f) || !Number.isInteger(properties.t)) continue;
    for (const year of [properties.f, properties.t + 1]) {
      if (year < -3400 || year > 2024 || year === 0) continue;
      eventYears.add(year);
      eventReferences++;
    }
  }
}
const YEARS = Array.from(new Set([...legacy, ...eventYears])).sort((a, b) => a - b);
if (YEARS.length > 1000 || YEARS[0] !== -3400 || YEARS.at(-1) !== 2024 || new Set(YEARS).size !== YEARS.length) {
  throw new Error(`Timeline must contain at most 1,000 unique years from 3400 BCE through 2024 CE; got ${YEARS.length}`);
}
const generatedHeader = [
  '// Generated by build_historical_worlds.js from the pinned History Atlas snapshot.',
  '#pragma once',
  `static const int HISTORY_YEARS[] = {${YEARS.slice().reverse().join(',')}};`,
  'static const int HISTORY_YEAR_COUNT = (int)(sizeof(HISTORY_YEARS) / sizeof(HISTORY_YEARS[0]));',
  ''
].join('\n');
fs.writeFileSync(path.join(__dirname, 'history_years.generated.h'), generatedHeader, 'utf8');
for (const name of fs.readdirSync(outputDirectory)) {
  const match = /^world-(-?\d+)\.json$/.exec(name);
  if (match && Number(match[1]) !== 2026 && !YEARS.includes(Number(match[1]))) fs.unlinkSync(path.join(outputDirectory, name));
}
console.log(JSON.stringify({ timelineYears: YEARS.length, boundaryEventYears: eventYears.size, boundaryReferences: eventReferences, preservedPreviousMilestones: legacy.length, earliest: YEARS[0], latest: YEARS.at(-1), maximum: 1000 }));
if (process.argv.includes('--timeline-only')) {
  delete globalThis.__historyBase;
  process.exit(0);
}
for (const year of YEARS) buildYear(year, base, topologies, outputDirectory);
delete globalThis.__historyBase;
