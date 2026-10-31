#define NOMINMAX
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Point { double x, y; };
struct Ring {
  std::vector<Point> points;
  double minY, maxY, signedArea;
};
struct Feature {
  std::string code, name;
  std::string nameZh;
  int mapColor;
  double labelX, labelY;
  std::vector<Ring> rings;
};
struct Field { std::string name; int offset, width; };

static uint32_t readU32LE(const std::vector<unsigned char>& b, size_t p) {
  if (p + 4 > b.size()) throw std::runtime_error("unexpected end of file");
  return (uint32_t)b[p] | ((uint32_t)b[p + 1] << 8) | ((uint32_t)b[p + 2] << 16) | ((uint32_t)b[p + 3] << 24);
}
static uint32_t readU32BE(const std::vector<unsigned char>& b, size_t p) {
  if (p + 4 > b.size()) throw std::runtime_error("unexpected end of file");
  return ((uint32_t)b[p] << 24) | ((uint32_t)b[p + 1] << 16) | ((uint32_t)b[p + 2] << 8) | (uint32_t)b[p + 3];
}
static double readDoubleLE(const std::vector<unsigned char>& b, size_t p) {
  if (p + 8 > b.size()) throw std::runtime_error("unexpected end of shapefile");
  uint64_t bits = 0;
  for (int i = 7; i >= 0; --i) bits = (bits << 8) | b[p + (size_t)i];
  double value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}
static std::vector<unsigned char> readBytes(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open " + path);
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}
static std::string trimField(const unsigned char* p, size_t n) {
  size_t first = 0, last = n;
  while (first < last && (p[first] == ' ' || p[first] == '\0')) ++first;
  while (last > first && (p[last - 1] == ' ' || p[last - 1] == '\0')) --last;
  return std::string((const char*)p + first, last - first);
}
static std::string jsonEscape(const std::string& value) {
  std::ostringstream out;
  for (unsigned char c : value) {
    if (c == '"') out << "\\\"";
    else if (c == '\\') out << "\\\\";
    else if (c == '\n') out << "\\n";
    else if (c == '\r') out << "\\r";
    else if (c == '\t') out << "\\t";
    else if (c < 0x20) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c << std::dec;
    else out << (char)c;
  }
  return out.str();
}

static std::vector<Feature> readFeatures(const std::string& shpPath, const std::string& dbfPath) {
  const auto shp = readBytes(shpPath);
  const auto dbf = readBytes(dbfPath);
  if (shp.size() < 100 || readU32BE(shp, 0) != 9994 || readU32LE(shp, 32) != 5)
    throw std::runtime_error("expected an ESRI Polygon shapefile");
  if (dbf.size() < 33) throw std::runtime_error("invalid DBF header");
  const uint32_t count = readU32LE(dbf, 4);
  const uint16_t headerLength = (uint16_t)(dbf[8] | (dbf[9] << 8));
  const uint16_t recordLength = (uint16_t)(dbf[10] | (dbf[11] << 8));
  std::vector<Field> fields;
  int fieldOffset = 1;
  for (size_t p = 32; p + 32 <= headerLength - 1; p += 32) {
    if (dbf[p] == 0x0d) break;
    std::string name = trimField(dbf.data() + p, 11);
    int width = dbf[p + 16];
    if (!name.empty() && width > 0) fields.push_back(Field{name, fieldOffset, width});
    fieldOffset += width;
  }
  auto field = [&](uint32_t row, const char* name) -> std::string {
    for (const auto& f : fields) if (f.name == name) {
      size_t p = (size_t)headerLength + (size_t)row * recordLength + (size_t)f.offset;
      if (p + (size_t)f.width > dbf.size()) throw std::runtime_error("truncated DBF row");
      return trimField(dbf.data() + p, (size_t)f.width);
    }
    throw std::runtime_error(std::string("missing DBF field ") + name);
  };
  const int rows = 560, columns = 1120;
  const double cellHeight = 180.0 / rows, cellWidth = 360.0 / columns;
  std::vector<Feature> result;
  result.reserve(count);
  size_t offset = 100;
  for (uint32_t row = 0; row < count; ++row) {
    if (offset + 8 > shp.size()) throw std::runtime_error("shapefile ended before DBF records");
    uint32_t contentBytes = readU32BE(shp, offset + 4) * 2;
    size_t content = offset + 8;
    if (content + contentBytes > shp.size() || contentBytes < 4) throw std::runtime_error("invalid shapefile record length");
    offset = content + contentBytes;
    std::string name = field(row, "ADMIN"), code = field(row, "ADM0_A3"), nameZh = field(row, "NAME_ZH"), colorText = field(row, "MAPCOLOR7");
    if (name.empty() || code.empty() || colorText.empty()) continue;
    int mapColor = std::atoi(colorText.c_str());
    if (mapColor < 1 || mapColor > 7) throw std::runtime_error("MAPCOLOR7 is outside 1..7 for " + name);
    std::string labelXText=field(row,"LABEL_X"),labelYText=field(row,"LABEL_Y");
    double labelX=labelXText.empty()?std::numeric_limits<double>::quiet_NaN():std::strtod(labelXText.c_str(),nullptr);
    double labelY=labelYText.empty()?std::numeric_limits<double>::quiet_NaN():std::strtod(labelYText.c_str(),nullptr);
    Feature item{code, name, nameZh, mapColor, labelX, labelY, {}};
    uint32_t shapeType = readU32LE(shp, content);
    if (shapeType == 0) { result.push_back(std::move(item)); continue; }
    if (shapeType != 5 || contentBytes < 44) throw std::runtime_error("unexpected shape type in country layer");
    uint32_t partCount = readU32LE(shp, content + 36), pointCount = readU32LE(shp, content + 40);
    size_t partsAt = content + 44, pointsAt = partsAt + (size_t)partCount * 4;
    if (partCount == 0 || pointCount < 4 || pointsAt + (size_t)pointCount * 16 > content + contentBytes)
      throw std::runtime_error("invalid polygon ring layout");
    std::vector<uint32_t> starts(partCount + 1);
    for (uint32_t i = 0; i < partCount; ++i) starts[i] = readU32LE(shp, partsAt + (size_t)i * 4);
    starts[partCount] = pointCount;
    for (uint32_t part = 0; part < partCount; ++part) {
      uint32_t begin = starts[part], end = starts[part + 1];
      if (begin >= end || end > pointCount) throw std::runtime_error("invalid ring point range");
      Ring ring; ring.minY = std::numeric_limits<double>::infinity(); ring.maxY = -ring.minY; ring.signedArea = 0;
      ring.points.reserve(end - begin);
      double previousX = 0;
      for (uint32_t i = begin; i < end; ++i) {
        double x = readDoubleLE(shp, pointsAt + (size_t)i * 16), y = readDoubleLE(shp, pointsAt + (size_t)i * 16 + 8);
        if (!ring.points.empty()) {
          while (x - previousX > 180.0) x -= 360.0;
          while (x - previousX < -180.0) x += 360.0;
        }
        ring.points.push_back(Point{x, y}); previousX = x;
        ring.minY = std::min(ring.minY, y); ring.maxY = std::max(ring.maxY, y);
      }
      for (size_t i = 0; i < ring.points.size(); ++i) {
        const Point& a = ring.points[i]; const Point& b = ring.points[(i + 1) % ring.points.size()];
        ring.signedArea += a.x * b.y - b.x * a.y;
      }
      item.rings.push_back(std::move(ring));
    }
    result.push_back(std::move(item));
  }
  if (result.size() < 200 || result.size() > 400) throw std::runtime_error("unexpected number of country records");
  return result;
}

static void writeSpan(std::vector<uint16_t>& cells, int row, double left, double right, uint16_t id, bool add) {
  const int columns = 1120;
  if (!(right > left)) return;
  for (int worldShift = -2; worldShift <= 2; ++worldShift) {
    double a = std::max(-180.0, left + worldShift * 360.0);
    double b = std::min(180.0, right + worldShift * 360.0);
    if (!(b > a)) continue;
    int first = std::max(0, (int)std::floor((a + 180.0) / 360.0 * columns) - 1);
    int last = std::min(columns - 1, (int)std::ceil((b + 180.0) / 360.0 * columns));
    for (int col = first; col <= last; ++col) {
      double center = -180.0 + (col + 0.5) * 360.0 / columns;
      if (center < a || center >= b) continue;
      uint16_t& cell = cells[(size_t)row * columns + col];
      if (add) cell = id;
      else if (cell == id) cell = 0;
    }
  }
}
static void rasterizeRing(const Ring& ring, uint16_t id, bool add, std::vector<uint16_t>& cells) {
  const int rows = 560;
  if (ring.points.size() < 4) return;
  for (int row = 0; row < rows; ++row) {
    double latitude = 90.0 - (row + 0.5) * 180.0 / rows;
    if (latitude < ring.minY || latitude >= ring.maxY) continue;
    std::vector<double> crossings;
    for (size_t i = 0, j = ring.points.size() - 1; i < ring.points.size(); j = i++) {
      const Point& a = ring.points[j]; const Point& b = ring.points[i];
      if ((a.y > latitude) == (b.y > latitude)) continue;
      crossings.push_back(a.x + (latitude - a.y) * (b.x - a.x) / (b.y - a.y));
    }
    std::sort(crossings.begin(), crossings.end());
    for (size_t i = 0; i + 1 < crossings.size(); i += 2) writeSpan(cells, row, crossings[i], crossings[i + 1], id, add);
  }
}
static void rasterize(const std::vector<Feature>& features, std::vector<uint16_t>& cells) {
  for (size_t i = 0; i < features.size(); ++i) {
    const uint16_t id = (uint16_t)(i + 1);
    const Feature& f = features[i];
    for (const auto& ring : f.rings) if (ring.signedArea < 0) rasterizeRing(ring, id, true, cells);
    for (const auto& ring : f.rings) if (ring.signedArea > 0) rasterizeRing(ring, id, false, cells);
  }
}
static size_t addRepresentativePoints(const std::vector<Feature>& features, std::vector<uint16_t>& cells) {
  std::vector<size_t> occupied(features.size(), 0);
  for (uint16_t id : cells) if (id > 0 && id <= features.size()) ++occupied[id - 1];
  std::set<size_t> representativeCells;
  size_t inserted = 0;
  for (size_t i = 0; i < features.size(); ++i) {
    const Feature& f = features[i];
    if (occupied[i] || f.rings.empty() || !std::isfinite(f.labelX) || !std::isfinite(f.labelY)
      || f.labelX < -180.0 || f.labelX > 180.0 || f.labelY < -90.0 || f.labelY > 90.0) continue;
    int row = std::min(559, std::max(0, (int)std::floor((90.0 - f.labelY) / 180.0 * 560.0)));
    int col = std::min(1119, std::max(0, (int)std::floor((f.labelX + 180.0) / 360.0 * 1120.0)));
    size_t cell = (size_t)row * 1120 + col;
    if (representativeCells.count(cell)) {
      double bestDistance = std::numeric_limits<double>::infinity(); size_t nearest = cells.size();
      for (int radius = 1; radius <= 4 && nearest == cells.size(); ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) for (int dx = -radius; dx <= radius; ++dx) {
          if (std::max(std::abs(dx), std::abs(dy)) != radius) continue;
          int candidateRow = row + dy, candidateCol = col + dx;
          if (candidateRow < 0 || candidateRow >= 560 || candidateCol < 0 || candidateCol >= 1120) continue;
          size_t candidate = (size_t)candidateRow * 1120 + candidateCol;
          if (representativeCells.count(candidate)) continue;
          double latitudeWeight = std::max(0.05, std::cos(f.labelY * 3.14159265358979323846 / 180.0));
          double distance = (dx * latitudeWeight) * (dx * latitudeWeight) + (double)dy * dy;
          if (distance < bestDistance) { bestDistance = distance; nearest = candidate; }
        }
      }
      if (nearest != cells.size()) cell = nearest;
    }
    representativeCells.insert(cell);
    cells[cell] = (uint16_t)(i + 1);
    ++inserted;
  }
  return inserted;
}
static std::string colorHex(int category) {
  static const char* palette[] = {"", "#55A8B5", "#F1C75B", "#927BB8", "#E97870", "#70A7CE", "#E5A261", "#76B573"};
  return palette[category];
}

int main(int argc, char** argv) {
  try {
    if (argc != 4) { std::cerr << "Usage: build_world_2026 <countries.shp> <countries.dbf> <output.json>\n"; return 2; }
    std::vector<Feature> features = readFeatures(argv[1], argv[2]);
    std::vector<uint16_t> cells(560u * 1120u, 0);
    rasterize(features, cells);
    size_t centerAssigned = 0; std::set<int> colors; std::vector<size_t> occupied(features.size(), 0);
    for (uint16_t id : cells) if (id > 0 && id <= features.size()) { ++centerAssigned; ++occupied[id - 1]; }
    size_t representativeFallbacks = addRepresentativePoints(features, cells);
    size_t assigned = 0; std::fill(occupied.begin(), occupied.end(), 0);
    for (uint16_t id : cells) if (id > 0 && id <= features.size()) { ++assigned; ++occupied[id - 1]; }
    for (const auto& f : features) colors.insert(f.mapColor);
    std::ofstream out(argv[3], std::ios::binary);
    if (!out) throw std::runtime_error("cannot create output module");
    out << "{\n  \"schemaVersion\":1,\n  \"moduleId\":\"world-2026\",\n  \"snapshotDate\":\"2026-10-30\",\n"
        "  \"gridRows\":560,\n  \"gridColumns\":1120,\n  \"assignmentRule\":\"Each 0.3214286 degree cell is assigned from its centre point inside a Natural Earth country polygon. If a country is smaller than a cell and receives no centre, its Natural Earth label point marks one representative cell; if two labels share a cell, the nearest unused cell is used. Unassigned ocean remains transparent.\",\n"
        "  \"sourceName\":\"Natural Earth Admin 0 - Countries 1:10m\",\n  \"sourceVersion\":\"5.1.1\",\n"
        "  \"sourceUrl\":\"https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-admin-0-countries/\",\n  \"license\":\"Public domain\",\n"
        "  \"sourceArchiveSha256\":\"CE1AC7036499A0EDD641FBC093CD209A98F96A49D2ECA8480AAACAD35138A7F6\",\n"
        "  \"paletteName\":\"Natural Earth MAPCOLOR7\",\n  \"countries\":[\n";
    for (size_t i = 0; i < features.size(); ++i) {
      const auto& f = features[i];
      out << "    {\"id\":" << (i + 1) << ",\"code\":\"" << jsonEscape(f.code) << "\",\"name\":\"" << jsonEscape(f.name)
          << "\",\"nameZh\":\"" << jsonEscape(f.nameZh)
          << "\",\"mapColor\":" << f.mapColor << ",\"color\":\"" << colorHex(f.mapColor) << "\"}" << (i + 1 == features.size() ? "\n" : ",\n");
    }
    out << "  ],\n  \"cells\":[\n";
    for (size_t i = 0; i < cells.size(); ++i) {
      if (i % 40 == 0) out << "    ";
      out << cells[i];
      if (i + 1 < cells.size()) out << ',';
      if (i % 40 == 39 || i + 1 == cells.size()) out << '\n';
    }
    out << "  ],\n  \"statistics\":{\"countryRecords\":" << features.size() << ",\"paletteCategories\":" << colors.size()
        << ",\"centerAssignedCells\":" << centerAssigned << ",\"representativePointFallbacks\":" << representativeFallbacks
        << ",\"assignedCells\":" << assigned << ",\"unassignedCells\":" << cells.size() - assigned << ",\"countriesWithCells\":";
    size_t occupiedCount = std::count_if(occupied.begin(), occupied.end(), [](size_t n) { return n > 0; });
    out << occupiedCount << "}\n}\n";
    out.close();
    if (!out) throw std::runtime_error("failed writing output module");
    std::cout << "MODULE: world-2026.json\nCOUNTRIES: " << features.size() << "\nGRID CELLS: " << cells.size()
              << "\nLAND-ASSIGNED CELLS: " << assigned << "\nOCEAN / UNASSIGNED CELLS: " << cells.size() - assigned
      << "\nCOUNTRY-CENTRE ASSIGNMENTS: " << centerAssigned << "\nREPRESENTATIVE-POINT FALLBACKS: " << representativeFallbacks
      << "\nCOUNTRIES WITH CELLS: " << occupiedCount << "\nPALETTE CATEGORIES: " << colors.size() << "\n";
    return assigned > 0 && occupiedCount >= features.size() - 5 ? 0 : 1;
  } catch (const std::exception& ex) {
    std::cerr << "World 2026 build failed: " << ex.what() << "\n";
    return 1;
  }
}
