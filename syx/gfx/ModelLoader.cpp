#include "Precompile.h"
#include "ModelLoader.h"
#include "Model.h"

using namespace Syx;

enum class CommandType : uint8_t {
  Vertex,
  VertexNormal,
  VertexParam,
  Comment,
  Texture,
  Face,
  Unknown
};

static CommandType _getCommandType(const char* type) {
  CommandType result = CommandType::Unknown;
  switch(*type) {
    case '#': return CommandType::Comment;
    case 'f': return CommandType::Face;
    case 'v': {
      switch(type[1]) {
        case ' ': return CommandType::Vertex;
        case 'n': return CommandType::VertexNormal;
        case 't': return CommandType::Texture;
        case 'p': return CommandType::VertexParam;
      }
    }
  }
  return result;
}

static bool _isNumber(char in) {
  return (in >= '0' && in <= '9') || in == '+' || in == '-';
}

static void _stripNumber(const char* line, int& curIndex) {
  while(_isNumber(line[curIndex]) && line[curIndex] != 0)
    ++curIndex;
}

static void _stripNonNumber(const char* line, int& curIndex) {
  while(!_isNumber(line[curIndex]) && line[curIndex] != 0)
    ++curIndex;
}

static void _stripNonWhitespace(const char* line, int& curIndex) {
  while(line[curIndex] != ' ' && line[curIndex] != 0)
    ++curIndex;
}

static Vec3 _readVec(const char* line, float defaultW, Vec3* second = nullptr, Vec3* third = nullptr) {
  int curIndex = 2;
  int foundNumbers = 0;
  Vec3 result(0.0f, 0.0f, 0.0f, defaultW);
  if(second) {
    *second = result;
    if(third)
      *third = result;
  }

  while(line[curIndex] != 0 && foundNumbers < 4) {
    _stripNonNumber(line, curIndex);
    if(line[curIndex]) {
      result[foundNumbers] = static_cast<float>(atof(&line[curIndex]));
      if(second) {
        //Given 1/2/3 Skip past first number (1), then the / to read 2
        _stripNumber(line, curIndex);
        if(line[curIndex] == '/') {
          ++curIndex;
          (*second)[foundNumbers] = static_cast<float>(atof(&line[curIndex]));

          //Same as above except for the third character
          if(third) {
            _stripNumber(line, curIndex);
            if(line[curIndex] == '/') {
              ++curIndex;
              (*third)[foundNumbers] = static_cast<float>(atof(&line[curIndex]));
            }
          }
        }
      }
      ++foundNumbers;
    }
    _stripNonWhitespace(line, curIndex);
  }
  return result;
}

bool ModelLoader::_readLine() {
  const size_t maxLineSize = 100;
  char line[maxLineSize];
  mStream.getline(line, maxLineSize);
  bool parsing = true;
  if(!mStream.good() || mStream.gcount() == 0)
    parsing = false;

  Vec3 v;
  CommandType cType = _getCommandType(line);
  switch(cType) {
    case CommandType::Vertex:
      v = _readVec(line, 1.0f);
      mVerts.push_back({v.x, v.y, v.z});
      break;
    case CommandType::Texture:
      v = _readVec(line, 1.0f);
      mUVs.push_back({v.x, v.y});
      break;
    case CommandType::VertexNormal:
      v = _readVec(line, 1.0f);
      mNormals.push_back({v.x, v.y, v.z});
      break;
    case CommandType::Face: {
      Vec3 vert, uv, normal;
      v = _readVec(line, -1.0f, &uv, &normal);
      VertLookup verts[4];
      for(int i = 0; i < 4; ++i) {
        //-1 all over because they aren't 0 indexed
        verts[i] = VertLookup(static_cast<size_t>(vert[i]) - 1, static_cast<size_t>(normal[i]) - 1, static_cast<size_t>(uv[i]) - 1);
      }

      size_t a = _getVertIndex(verts[0]);
      size_t b = _getVertIndex(verts[1]);
      size_t c = _getVertIndex(verts[2]);

      if(v.w < 0.0f) {
        _addTri(a, b, c);
      }
      else {
        size_t d = _getVertIndex(verts[3]);
        _addTri(a, b, d);
        _addTri(b, c, d);
      }
      break;
    }
  }
  return parsing;
}

void ModelLoader::_reset() {
  mModel = nullptr;
  mVerts.clear();
  mNormals.clear();
  mUVs.clear();
  mVertToIndex.clear();
}

std::unique_ptr<Model> ModelLoader::loadModel(const std::string& objFile) {
  _reset();
  mStream = std::ifstream(objFile);
  if(!mStream.good())
    return nullptr;

  mModel = std::make_unique<Model>();
  bool parsing = true;
  do {
    parsing = _readLine();
  }
  while(parsing);

  return std::move(mModel);
}

size_t ModelLoader::_getVertIndex(const VertLookup& lookup) {
  size_t prevSize = mVertToIndex.size();
  size_t& index = mVertToIndex[lookup];
  size_t postSize = mVertToIndex.size();
  //If size changed, this was new, so create new vertex and add to model
  if(prevSize != postSize) {
    const V3& vert = mVerts[lookup.mVert];
    const V3& normal = mNormals[lookup.mNormal];
    const V2& uv = mUVs[lookup.mUV];
    Vertex v(vert.x, vert.y, vert.z, normal.x, normal.y, normal.z, uv.x, uv.y);
    index = mModel->mVerts.size();
    mModel->mVerts.emplace_back(v);
  }
  return 0;
}

void ModelLoader::_addTri(size_t a, size_t b, size_t c) {
  mModel->mIndices.push_back(a);
  mModel->mIndices.push_back(b);
  mModel->mIndices.push_back(c);
}