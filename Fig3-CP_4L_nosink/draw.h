#ifndef DRAW_H
#define DRAW_H

#include <drawing/drawable.h>
#include <drawing/shader.h>
#include <util/palette.h>

#include <QMutex>
#include <QMutexLocker>

#include <limits>

#include "structure.h"

using cellflips::out;

enum UserActiveTextures {
  AT_CELL_COLORS = drawing::Drawable::AT_USER+1
};

template <typename Model>
class DrawCell : public drawing::Drawable
{
public:
  typedef util::Palette::Color Colorf;

  DrawCell(Model *model_, const Tissue& T_)
    : drawing::Drawable(model_)
    , model(model_)
    , T(T_)
    , _data(model_, T_)
    , _new_data(model_, T_)
  {
    setObjectName("Cells");
    setCanMove(false);
    setFilename("view.v");
    reread();
  }

  void init(Viewer *, drawing::Shader* t) override
  {
    trans = cellflips::make_unique<drawing::Shader>(*t);
    trans->changeVertexShader(VS_COLOR, "color.vert");
    trans->clearGeometryShader();
    trans->addGeometryShader(":/shaders/opengl32_geometry/layout.geom");
    trans->addGeometryShader("peeling_surf.geom");
    trans->setUniform("peeling", drawing::GLSLValue(true));
    trans->setUniform("cellColors", drawing::GLSLValue(AT_CELL_COLORS));
    trans->setUniform("transparent", drawing::GLSLValue(true));
    //trans->setVerbosity(5);

    opaque = cellflips::make_unique<drawing::Shader>(*trans);
    opaque->setUniform("peeling", drawing::GLSLValue(false));
    opaque->setUniform("transparent", drawing::GLSLValue(false));

    if(not trans->setupShaders())
      out << "Warning, cannot compile shaders" << endl;

    if(not opaque->setupShaders())
      out << "Warning, cannot compile shaders" << endl;

    // Shader code:
    if(false) {
      out << "Vertex shader:" << endl;
      QStringList lines = trans->vertexShader().split("\n");
      for(size_t i = 0 ; i < lines.size() ; ++i)
        out << QString::number(i).leftJustified(6, ' ') << lines[i] << endl;

      out << "\nGeometry shader:" << endl;
      lines = trans->geometryShader().split("\n");
      for(size_t i = 0 ; i < lines.size() ; ++i)
        out << QString::number(i).leftJustified(6, ' ') << lines[i] << endl;

      out << "\nFragment shader:" << endl;
      lines = trans->fragmentShader().split("\n");
      for(size_t i = 0 ; i < lines.size() ; ++i)
        out << QString::number(i).leftJustified(6, ' ') << lines[i] << endl;

      out << endl;
    }

    // Allocate the cell color texture buffer
    glGenTextures(1, &_cellColorTexId);
    glGenBuffers(1, &_cellColorBufId);

    // Create the buffer
    Colorf empty[2] = { Colorf(1,0,0,1), Colorf(1,1,1,1) };
    glBindBuffer(GL_TEXTURE_BUFFER, _cellColorBufId);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(Colorf) * 2, empty, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Associate the texture to the buffer
    glBindTexture(GL_TEXTURE_BUFFER, _cellColorTexId);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _cellColorBufId);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
  }

  void finalize(Viewer*) override
  {
    glDeleteTextures(1, &_cellColorTexId);
    _cellColorTexId = 0;
    glDeleteBuffers(1, &_cellColorBufId);
    _cellColorBufId = 0;
  }

  void updateGeometry()
  {
    out << "updateGeometry(), cellSizeProportion = " << cellSizeProportion << endl;
    Point3d pmin(HUGE_VAL), pmax(-HUGE_VAL);

    if(not T.empty()) {
      auto nb_cells = T.nbCells();
      size_t i = 0;
      for(const cell& c: T.cells()) {
        _new_data.addCell(c, cellSizeProportion);
      }
      for(const ccvertex& v: T.vertices()) {
        const auto& p = v->pos;
        if(p.x() < pmin.x()) pmin.x() = p.x();
        if(p.y() < pmin.y()) pmin.y() = p.y();
        if(p.z() < pmin.z()) pmin.z() = p.z();
        if(p.x() > pmax.x()) pmax.x() = p.x();
        if(p.y() > pmax.y()) pmax.y() = p.y();
        if(p.z() > pmax.z()) pmax.z() = p.z();
      }
      out << "    ... with " << T.nbCells() << " cells" << endl;
      _bsphere = BSphere((pmin+pmax)/2, util::norm(pmax-pmin)/2);
    } else {
      _bsphere = BSphere({0,0,0}, 1.);
      out << "    ... without cells" << endl;
    }

    using std::swap;

    {
      QMutexLocker updating(&_lock);
      _data.swap(_new_data);
      _new_data.clear();
      _needLoadingTexture = true;
    }

    if(false) {
      GLuint max_cell_id = 0;
      for(GLuint cid: _data.cellid) {
        if(cid > max_cell_id)
          max_cell_id = cid;
      }
      out << "# triangles = " << _data.triangles.size() << endl
          << "# positions = " << _data.positions.size() << endl
          << "# colors = "    << _data.colors.size()    << endl
          << "# cells = "     << (max_cell_id+1)        << endl;
    }

    _new_data.clear();
  }

  void loadTexture()
  {
    if(_needLoadingTexture and _cellColorTexId > 0 and not _data.colors.empty()) {
      _needLoadingTexture = false;
      // Load data into cellColor buffer
      glBindBuffer(GL_TEXTURE_BUFFER, _cellColorBufId);
      glBufferData(GL_TEXTURE_BUFFER, sizeof(Colorf) * _data.colors.size(), _data.colors.data(), GL_DYNAMIC_DRAW);
      glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }
  }

  void updateColors()
  {
    _new_data.colors.clear();
    _new_data.colors.reserve(_data.cells.size());

    for(const cell& c: _data.cells) {
      auto col = model->cellColor(c);
      _new_data.colors.push_back(col);
    }

    {
      QMutexLocker updating(&_lock);
      std::swap(_data.colors, _new_data.colors);
      _new_data.clear();
      _needLoadingTexture = true;
    }
  }


  void reread() override
  {
    util::Parms parms(QString::fromStdString(filename()));

    parms("View", "CellSizeProportion", cellSizeProportion);
    parms("View", "TransparentCells", transparentCells);

    updateGeometry();
  }

  void drawOpaque(Viewer *) override
  {
    if(not transparentCells) {
      drawArrays(opaque.get());
    }
  }

  void drawTransparent(Viewer *) override
  {
    if(transparentCells) {
      drawArrays(trans.get());
    }
  }

  BSphere boundingSphere() const override
  {
    return _bsphere;
  }

protected:
  void drawArrays(drawing::Shader* shader)
  {
    QMutexLocker drawing(&_lock);
    loadTexture();
    //out << "===> Start drawArrays" << endl;
    if(not _data.triangles.empty())
    {
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
      glFrontFace(GL_CCW);
      glColor3f(1,1,1);

      shader->useShaders();

      GLuint cellId = shader->attribLocation("vertexColorIdx");
      REPORT_GL_ERROR("attribLocation");

      shader->activeTexture(AT_CELL_COLORS);
      glBindTexture(GL_TEXTURE_BUFFER, _cellColorTexId);
      shader->activeTexture(0);
      REPORT_GL_ERROR("load texture");

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_NORMAL_ARRAY);
      REPORT_GL_ERROR("Set vertex and normal client state");
      glEnableVertexAttribArray(cellId);
      REPORT_GL_ERROR("Set client state");

      glVertexPointer(3, GL_DOUBLE, 0, _data.positions.data());
      glNormalPointer(GL_DOUBLE, 0, _data.normals.data());
      glVertexAttribIPointer(cellId, 1, GL_UNSIGNED_INT, 0, _data.cellid.data());

      glDrawElements(GL_TRIANGLES, _data.triangles.size(), GL_UNSIGNED_INT, _data.triangles.data());
      REPORT_GL_ERROR("glDrawElements");

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_NORMAL_ARRAY);
      glDisableVertexAttribArray(cellId);
      REPORT_GL_ERROR("disable client state");

      shader->activeTexture(AT_CELL_COLORS);
      glBindTexture(GL_TEXTURE_BUFFER, 0);
      shader->activeTexture(0);

      shader->stopUsingShaders();
      REPORT_GL_ERROR("drawArrays");
    }
    //out << "===> End drawArrays" << endl;
  }

  Model *model;
  const Tissue& T;

  struct RenderingData {
    RenderingData(Model* model_, const Tissue& T_)
      : model(model_)
      , T(T_)
    { }

    void clear() {
      positions.clear();
      normals.clear();
      cellid.clear();
      triangles.clear();
      cells.clear();
      colors.clear();
    }

    void swap(RenderingData& other) {
      using std::swap;
      swap(positions, other.positions);
      swap(normals, other.normals);
      swap(cellid, other.cellid);
      swap(triangles, other.triangles);
      swap(cells, other.cells);
      swap(colors, other.colors);
    }

    void addCell(const cell& c, double cellSizeProportion)
    {
      const Point3d& pos = c->pos;

      size_t cid = cells.size();

      cells.push_back(c);
      colors.push_back(model->cellColor(c));

      for(auto of: T.boundary(+c)) {
        const Point3d& n = int(of.orientation())*of->normal;
        auto vs = T.orderedVertices(of);
        triangles.reserve(triangles.size() + 3*vs.size());
        size_t start_vid = positions.size();
        Point3d p;
        for(auto v: vs) {
          cellid.push_back(cid);
          normals.push_back(n);
          auto vp = cellSizeProportion*(v->pos - pos) + pos;
          positions.push_back(vp);
          p += vp;
        }
        p /= vs.size();
        size_t fid = positions.size();
        positions.push_back(pos + (of->pos - pos) * cellSizeProportion);
        cellid.push_back(cid);
        normals.push_back(n);

        size_t prev_id = start_vid + vs.size() - 1;
        for(size_t cur_id = start_vid ; cur_id < start_vid + vs.size() ; ++cur_id) {
          triangles.push_back(fid);
          triangles.push_back(prev_id);
          triangles.push_back(cur_id);
          prev_id = cur_id;
        }
      }
    }

    Model* model;
    const Tissue& T;

    // For each point
    std::vector<Point3d> positions, normals;
    std::vector<GLuint> cellid;

    // Index in the previous arrays defining triangles
    std::vector<GLuint> triangles;

    // Mapping cells to positions in the color buffer
    std::vector<cell> cells;

    // Buffer giving the color of each cell
    std::vector<Colorf> colors;
  } _data, _new_data;

  GLuint _cellColorTexId = 0;
  GLuint _cellColorBufId = 0;

  BSphere _bsphere;

  double cellSizeProportion = 0.9;
  bool transparentCells = true;
  bool _needLoadingTexture = false;

  std::unique_ptr<drawing::Shader> trans, opaque;
  QMutex _lock;
};

template <typename Model>
class DrawPIN : public drawing::Drawable
{
public:
  typedef util::Palette::Color Colorf;

  DrawPIN(Model* _model, const Tissue& _T)
    : drawing::Drawable(_model)
    , model(_model)
    , T(_T)
    , _data(model, T)
    , _new_data(model, T)
  {
    setObjectName("PIN");
    setCanMove(false);
    setFilename("view.v");
    reread();
  }

  void init(Viewer *, drawing::Shader* t) override
  {
    trans = cellflips::make_unique<drawing::Shader>(*t);
    trans->changeVertexShader(VS_COLOR, "color.vert");
    trans->clearGeometryShader();
    trans->addGeometryShader("layout_PIN.geom");
    trans->addGeometryShader("peeling_surf_PIN.geom");
    trans->setUniform("peeling", drawing::GLSLValue(true));
    trans->setUniform("faceColors", drawing::GLSLValue(AT_CELL_COLORS));
    trans->setUniform("transparent", drawing::GLSLValue(true));
    //trans->setVerbosity(5);

    opaque = cellflips::make_unique<drawing::Shader>(*trans);
    opaque->setUniform("peeling", drawing::GLSLValue(false));
    opaque->setUniform("transparent", drawing::GLSLValue(false));

    if(not trans->setupShaders())
      out << "Warning, cannot compile shaders" << endl;

    if(not opaque->setupShaders())
      out << "Warning, cannot compile shaders" << endl;

    // Shader code:
    if(false) {
      out << "Vertex shader:" << endl;
      QStringList lines = trans->vertexShader().split("\n");
      for(size_t i = 0 ; i < lines.size() ; ++i)
        out << QString::number(i).leftJustified(6, ' ') << lines[i] << endl;

      out << "\nGeometry shader:" << endl;
      lines = trans->geometryShader().split("\n");
      for(size_t i = 0 ; i < lines.size() ; ++i)
        out << QString::number(i).leftJustified(6, ' ') << lines[i] << endl;

      out << "\nFragment shader:" << endl;
      lines = trans->fragmentShader().split("\n");
      for(size_t i = 0 ; i < lines.size() ; ++i)
        out << QString::number(i).leftJustified(6, ' ') << lines[i] << endl;

      out << endl;
    }

    // Allocate the cell color texture buffer
    glGenTextures(1, &_faceColorTexId);
    glGenBuffers(1, &_faceColorBufId);

    // Create the buffer
    Colorf empty[2] = { Colorf(1,0,0,1), Colorf(1,1,1,1) };
    glBindBuffer(GL_TEXTURE_BUFFER, _faceColorBufId);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(Colorf) * 2, empty, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Associate the texture to the buffer
    glBindTexture(GL_TEXTURE_BUFFER, _faceColorTexId);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _faceColorBufId);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
  }

  void finalize(Viewer*) override
  {
    glDeleteTextures(1, &_faceColorTexId);
    _faceColorTexId = 0;
    glDeleteBuffers(1, &_faceColorBufId);
    _faceColorBufId = 0;
  }

  void updateGeometry()
  {
    Point3d pmin(HUGE_VAL), pmax(-HUGE_VAL);

    if(not T.empty()) {
      for(const cell& c: T.cells()) {
        for(const auto& of: T.boundary(+c)) {
          _new_data.addPIN(c, of, cellSizeProportion);
        }
      }

      for(const ccvertex& v: T.vertices()) {
        const auto& p = v->pos;
        if(p.x() < pmin.x()) pmin.x() = p.x();
        if(p.y() < pmin.y()) pmin.y() = p.y();
        if(p.z() < pmin.z()) pmin.z() = p.z();
        if(p.x() > pmax.x()) pmax.x() = p.x();
        if(p.y() > pmax.y()) pmax.y() = p.y();
        if(p.z() > pmax.z()) pmax.z() = p.z();
      }
      _bsphere = BSphere((pmin+pmax)/2, util::norm(pmax-pmin)/2);
    } else
      _bsphere = BSphere(Point3d(0,0,0), 1.);

    {
      QMutexLocker updating(&_lock);
      _data.swap(_new_data);
      _new_data.clear();
    }

  }

  void updateColors()
  {
    _new_data.colors.clear();
    _new_data.colors.reserve(_data.faces.size());

    for(const auto& of: _data.faces) {
      auto col = model->faceColor(of);
      _new_data.colors.push_back(col);
    }

    {
      QMutexLocker updating(&_lock);
      std::swap(_data.colors, _new_data.colors);
      _new_data.clear();
      _needLoadingTexture = true;
    }
  }

  void reread() override
  {
    util::Parms parms(QString::fromStdString(filename()));

    parms("View", "CellSizeProportion", cellSizeProportion);
    parms("View", "PINSize", PINSize);
    parms("View", "TransparentPIN", transparentPIN);

    out << "PINSize: " << PINSize << endl;

    updateGeometry();
  }

  void drawOpaque(Viewer *) override
  {
    if(not transparentPIN) {
      trans->useShaders();
      trans->setUniform_instant("peeling", drawing::GLSLValue(false));
      drawArrays(opaque.get());
      trans->stopUsingShaders();
    }
  }

  void drawTransparent(Viewer *) override
  {
    if(transparentPIN) {
      trans->useShaders();
      drawArrays(trans.get());
      trans->stopUsingShaders();
    }
  }

  BSphere boundingSphere() const override
  {
    return _bsphere;
  }

protected:

  void loadTexture()
  {
    if(_needLoadingTexture and _faceColorTexId > 0 and not _data.colors.empty()) {
      _needLoadingTexture = false;

      glBindBuffer(GL_TEXTURE_BUFFER, _faceColorBufId);
      glBufferData(GL_TEXTURE_BUFFER, sizeof(Colorf) * _data.colors.size(), _data.colors.data(), GL_DYNAMIC_DRAW);
      glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }
  }

  void drawArrays(drawing::Shader* shader)
  {
    QMutexLocker drawing(&_lock);
    loadTexture();
    //out << "===> Start drawArrays" << endl;
    if(not _data.triangles.empty())
    {
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
      glFrontFace(GL_CCW);
      glColor3f(1,1,1);

      shader->setUniform("PINSize", drawing::GLSLValue(float(PINSize)));

      shader->useShaders();

      CHECK_GL_ERROR(glEnable(GL_PRIMITIVE_RESTART));
      CHECK_GL_ERROR(glPrimitiveRestartIndex(restartIndex));

      GLuint faceId = shader->attribLocation("vertexColorIdx");
      REPORT_GL_ERROR("attribLocation");

      shader->activeTexture(AT_CELL_COLORS);
      glBindTexture(GL_TEXTURE_BUFFER, _faceColorTexId);
      shader->activeTexture(0);
      REPORT_GL_ERROR("load texture");

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_NORMAL_ARRAY);
      REPORT_GL_ERROR("Set vertex and normal client state");
      glEnableVertexAttribArray(faceId);
      REPORT_GL_ERROR("Set client state");

      glVertexPointer(3, GL_DOUBLE, 0, _data.positions.data());
      glNormalPointer(GL_DOUBLE, 0, _data.normals.data());
      glVertexAttribIPointer(faceId, 1, GL_INT, 0, _data.faceid.data());

      glDrawElements(GL_TRIANGLE_FAN, _data.triangles.size(), GL_UNSIGNED_INT, _data.triangles.data());
      REPORT_GL_ERROR("glDrawElements");

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_NORMAL_ARRAY);
      glDisableVertexAttribArray(faceId);
      REPORT_GL_ERROR("disable client state");

      shader->activeTexture(AT_CELL_COLORS);
      glBindTexture(GL_TEXTURE_BUFFER, 0);
      shader->activeTexture(0);

      shader->stopUsingShaders();
      glDisable(GL_PRIMITIVE_RESTART);

      REPORT_GL_ERROR("drawArrays");
    }
    //out << "===> End drawArrays" << endl;
  }

  Model* model;
  const Tissue& T;

  struct RenderingData {
    RenderingData(Model* model_, const Tissue& T_)
      : model(model_)
      , T(T_)
    { }

    void clear() {
      positions.clear();
      normals.clear();
      faceid.clear();
      triangles.clear();
      faces.clear();
      colors.clear();
    }

    void swap(RenderingData& other) {
      using std::swap;
      swap(positions, other.positions);
      swap(normals, other.normals);
      swap(faceid, other.faceid);
      swap(triangles, other.triangles);
      swap(faces, other.faces);
      swap(colors, other.colors);
    }

    void addPIN(const cell& c, const oriented_face& of, double cellSizeProportion)
    {
      //cellSizeProportion *= 1.01;
      const Point3d& pos = c->pos;

      size_t fid = faces.size();
      faces.push_back(of);
      auto col = model->faceColor(of);
      colors.push_back(col);

      // Normal to the surface
      auto fn = int(of.orientation()) * of->normal;

      // Ordered list of vertices
      auto vs = T.orderedVertices(of);

      auto fpid = positions.size();
      positions.push_back(pos + (of->pos-pos)*cellSizeProportion);
      normals.push_back(fn);
      faceid.push_back(fid);
      triangles.push_back(fpid);

      auto vid = positions.size();
      for(size_t i = 0 ; i < vs.size() ; ++i) {
        auto v = vs[i];
        auto n = vs[i+1 < vs.size() ? i+1 : 0];
        positions.push_back(pos + (v->pos - pos)*cellSizeProportion);
        faceid.push_back(fid);
        auto en = util::normalized((n->pos - v->pos) ^ fn);
        normals.push_back(en);
        triangles.push_back(vid++);
      }
      triangles.push_back(fpid+1);
      triangles.push_back(restartIndex);
    }

    Model* model;
    const Tissue& T;

    // For each point
    std::vector<Point3d> positions, normals;
    std::vector<GLuint> faceid;

    // Index in the previous arrays defining triangles
    std::vector<GLuint> triangles;

    // Mapping oriented faces to positions in the color buffer
    std::vector<oriented_face> faces;

    // Buffer giving the color of each cell
    std::vector<Colorf> colors;
  } _data, _new_data;

  BSphere _bsphere;

  double cellSizeProportion = 0.9, PINSize = 0.1;
  bool transparentPIN;

  static const GLuint restartIndex;

  GLuint _faceColorTexId = 0;
  GLuint _faceColorBufId = 0;
  bool _needLoadingTexture = false;

  std::unique_ptr<drawing::Shader> trans, opaque;
  QMutex _lock;
};

template <typename Model>
const GLuint DrawPIN<Model>::restartIndex = std::numeric_limits<GLuint>::max()-1;

#endif // DRAW_H

