#ifndef DRAWER_VVH
#define DRAWER_VVH

#include <util/gl.h>
#include <drawing/drawable.h>
#include <drawing/shader.h>

#include <QMutexLocker>
#include <QMutex>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QWidget>
//#include <QUiLoader>
#include <QGroupBox>
#include <QFile>

#include <geometry/geometry.h>
#include <util/palette.h>
#include <util/parms.h>
#include <util/mangling.h>

#include <cellflips/cellflips.h>

using geometry::Point3d;
using drawing::Drawable;
using drawing::Shader;

using qglviewer::Vec;

using util::Palette;
typedef util::Palette::Color Color;
using namespace cellflips;

enum ComplexDrawerUserActiveTextures
{
  CDAT_PALETTE = Drawable::AT_USER
};

struct GenCell : public Drawable
{
  GenCell(QMutex *m, QObject *parent)
    : Drawable(parent)
    , _mutex(m)
    , trans(0)
    , palTexId(0)
  {
    setCanDrag(false);
    setCanMove(false);
  }

  using Drawable::init;

  virtual void init(Viewer *, Shader *t, Shader * /*def*/)
  {
    trans = t;
  }

  QMutex* mutex()
  {
    return _mutex;
  }

  void setPalTexId(GLuint id)
  {
    palTexId = id;
  }

  QMutex *_mutex;
  Shader *trans;
  GLuint palTexId;
};

class Vertices : public GenCell
{
  Q_OBJECT
  public:
  Vertices(QMutex *m, QObject *parent)
    : GenCell(m, parent)
    , color(1,1,1,1)
    , pointSize(6)
  {
    this->setObjectName("Vertices");
  }

  void drawOpaque(Viewer *)
  {
    QMutexLocker lock(this->mutex());

    glDisable(GL_LIGHTING);
    glPointSize(pointSize);
    glColor3fv(color.c_data());
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, &pts.front());
    glDrawArrays(GL_POINTS, 0, pts.size());

/*
 *    if(showNormals)
 *    {
 *      opaque->useShaders();
 *
 *      opaque->setUniform_instant("normalSize", drawing::GLSLValue(normalSize));
 *      GLint loc = opaque->attribLocation("vnormal");
 *      glEnableVertexAttribArray(loc);
 *
 *      glLineWidth(normalThickness);
 *      glVertexAttribPointer(loc, 3, GL_DOUBLE, GL_FALSE, 0, &normals.front());
 *      glVertexPointer(3, GL_DOUBLE, 0, &starts.front());
 *      glDrawArrays(GL_LINES, 0, normals.size());
 *
 *      glDisableVertexAttribArray(loc);
 *      opaque->stopUsingShaders();
 *    }
 */

    glDisableClientState(GL_VERTEX_ARRAY);

    REPORT_GL_ERROR("Vertex::drawOpaque()");
  }

  void init(Viewer *, Shader *, Shader *def)
  {
    opaque = new Shader(*def);
    opaque->setUniform("peeling", drawing::GLSLValue(false));
    opaque->changeVertexShaderCode(Drawable::VS_POSITION,
                                   QString("in vec3 vnormal;\n"
                                           "uniform float normalSize;\n"
                                           "vec4 position()\n"
                                           "{\n"
                                           "  if(gl_Vertex.w == 0) return gl_Vertex;\n"
                                           "  else {\n"
                                           "    vec3 pos = gl_Vertex.xyz/gl_Vertex.w + normalSize*vnormal;\n"
                                           "    return vec4(pos,1.0);\n"
                                           "  }\n"
                                           "}\n")
                                  );
    opaque->setupShaders();
  }

  /*
  QWidget *controls()
  {
    QUiLoader l;
    QFile f("vertices.ui");
    if(!f.open(QIODevice::ReadOnly))
    {
      out << "Error opening vertices.ui" << endl;
      return 0;
    }
    QWidget *w = l.load(&f);
    QDoubleSpinBox *ps = w->findChild<QDoubleSpinBox*>("pointSize");
    if(!ps)
    {
      out << "Error, cannot find double spin box 'pointSize'" << endl;
      delete w;
      return 0;
    }
    GLfloat psr[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, psr);
    ps->setMinimum(psr[0]);
    ps->setMaximum(psr[1]);
    ps->setSingleStep(1.0);

    ps->setValue(pointSize);

    connect(ps, SIGNAL(valueChanged(double)), this, SLOT(changePointSize(double)));

    return w;
  }
  */

  Shader *opaque;
  Color color;
  float pointSize;
  std::vector<Point3d> pts;

  public slots:
  void changePointSize(double v)
  {
    pointSize = float(v);
    this->redraw();
  }

};

class Edges : public GenCell
{
  Q_OBJECT
  public:
  Edges(QMutex *m, QObject *parent)
    : GenCell(m, parent)
    , opaque(0)
    , lineWidth(2)
  {
    this->setObjectName("Edges");
  }

  void init(Viewer *, Shader *, Shader *)
  {
    opaque = new Shader();

    opaque->addVertexShaderCode(QString("in vec3 center;\n"
                                        "out vec4 vertexColor;\n"
                                        "out vec4 vertexClip;\n"
                                        "out vec4 vertexPos;\n"
                                        "void main()\n"
                                        "{\n"
                                        "  vertexPos = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n"
                                        "  vertexClip = gl_ModelViewMatrix * vec4(center,1);\n"
                                        "  vertexColor = gl_Color;\n"
                                        "}\n"));

    opaque->addGeometryShaderCode(QString("layout (lines) in;\n"
                                          "layout (line_strip, max_vertices=2) out;\n"
                                          "out gl_PerVertex {\n"
                                          "  vec4 gl_Position;\n"
                                          "  vec4 gl_FrontColor;\n"
                                          "  vec4 gl_ClipVertex;\n"
                                          "};\n"
                                          "\n"
                                          "in vec4 vertexColor[];\n"
                                          "in vec4 vertexClip[];\n"
                                          "in vec4 vertexPos[];\n"
                                          "centroid out vec4 fragColor;\n"
                                          "void main() {"
                                          "  for(int i = 0 ; i < 2 ; ++i) {\n"
                                          "     gl_Position = vertexPos[i];\n"
                                          "     gl_ClipVertex = vertexClip[i];\n"
                                          "     fragColor = vertexColor[i];\n"
                                          "     EmitVertex();\n"
                                          "   }\n"
                                          "   EndPrimitive();\n"
                                          "}"));


    opaque->addFragmentShaderCode(QString("centroid in vec4 fragColor;\n"
                                          "void main()\n"
                                          "{\n"
                                          "  gl_FragColor = fragColor;\n"
                                          "}\n"));
    if(not opaque->setupShaders())
      out << "Warning, cannot compile Edge opaque shaders" << endl;

  }

  void finalize(Viewer *)
  {
    opaque->cleanShaders();
    delete opaque;
    opaque = 0;
  }

  void drawOpaque(Viewer *)
  {
    QMutexLocker lock(this->mutex());
    //out << "Edges::drawOpaque()" << endl;

    opaque->useShaders();

    GLint loc = opaque->attribLocation("center");


    glDisable(GL_LIGHTING);
    glLineWidth(lineWidth);
    glColor3fv(color.c_data());

    glEnableVertexAttribArray(loc);
    glEnableClientState(GL_VERTEX_ARRAY);

    glVertexAttribPointer(loc, 3, GL_DOUBLE, GL_FALSE, 0, &centers.front());
    glVertexPointer(3, GL_DOUBLE, 0, &lines.front());

    glDrawArrays(GL_LINES, 0, lines.size());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableVertexAttribArray(loc);

    opaque->stopUsingShaders();

    REPORT_GL_ERROR("Edges::drawOpaque()");
  }

  /*
  QWidget *controls()
  {
    QUiLoader l;
    QFile f("edges.ui");
    if(!f.open(QIODevice::ReadOnly))
    {
      out << "Error opening faces.ui" << endl;
      return 0;
    }
    QWidget *w = l.load(&f);
    QDoubleSpinBox *lt = w->findChild<QDoubleSpinBox*>("lineThickness");
    if(!lt)
    {
      out << "Error, cannot find double spin box 'lineThickness'" << endl;
      delete w;
      return 0;
    }
    GLfloat lwr[2];
    glGetFloatv(GL_LINE_WIDTH_RANGE, lwr);
    lt->setMinimum(lwr[0]);
    lt->setMaximum(lwr[1]);
    lt->setSingleStep(1.0);

    lt->setValue(lineWidth);

    connect(lt, SIGNAL(valueChanged(double)), this, SLOT(changeLineWidth(double)));

    return w;
  }
  */

  Shader *opaque;
  Color color;
  float lineWidth;
  std::vector<Point3d> lines, centers;

  public slots:

  void changeLineWidth(double v)
  {
    lineWidth = float(v);
    redraw();
  }
};

class Faces : public GenCell
{
  Q_OBJECT
  public:

    Color normalColor;
    float normalThickness, normalSize;
    float faceTransparency;
    bool showNormals;
    std::vector<Point3d> center_faces, faces_normals;
    std::vector<Point3d> faces, faces_center, normals;
    std::vector<float> colors;
    Shader *opaque;

    Faces(QMutex *m, QObject *parent)
      : GenCell(m, parent)
      , normalThickness(1)
      , faceTransparency(0.7)
      , showNormals(false)
      , opaque(0)
    {
      this->setObjectName("Faces");
      this->setVisible(false);
    }

    void init(Viewer *, Shader *t, Shader *def)
    {
      trans = new Shader(*t);

      opaque = new Shader();

      opaque->addVertexShaderCode(QString("in vec3 center;\n"
                                          "in vec3 vnormal;\n"
                                          "out vec4 vertexColor;\n"
                                          "out vec4 vertexClip;\n"
                                          "out vec4 vertexPos;\n"
                                          "out vec3 vertexNormal;\n"
                                          "void main()\n"
                                          "{\n"
                                          "  vertexPos = gl_Vertex;\n"
                                          "  vertexNormal = vnormal;\n"
                                          "  vertexClip = gl_ModelViewMatrix * vec4(center,1);\n"
                                          "  vertexColor = gl_Color;\n"
                                          "}\n"));

      opaque->addGeometryShaderCode(QString("layout (points) in;\n"
                                            "layout (line_strip, max_vertices=2) out;\n"
                                            "out gl_PerVertex {\n"
                                            "  vec4 gl_Position;\n"
                                            "  vec4 gl_FrontColor;\n"
                                            "  vec4 gl_ClipVertex;\n"
                                            "};\n"
                                            "\n"
                                            "uniform float normalSize;\n"
                                            "in vec4 vertexColor[];\n"
                                            "in vec4 vertexClip[];\n"
                                            "in vec4 vertexPos[];\n"
                                            "in vec3 vertexNormal[];\n"
                                            "centroid out vec4 fragColor;\n"
                                            "void main() {"
                                            "   gl_Position = gl_ModelViewProjectionMatrix * vertexPos[0];\n"
                                            "   gl_ClipVertex = vertexClip[0];\n"
                                            "   fragColor = vertexColor[0];\n"
                                            "   EmitVertex();\n"
                                            "   vec4 npos = vertexPos[0];\n"
                                            "   if(npos.w != 0)\n"
                                            "      npos = vec4(npos.xyz / npos.w + normalSize * vertexNormal[0], 1);\n"
                                            "   gl_Position = gl_ModelViewProjectionMatrix * npos;\n"
                                            "   gl_ClipVertex = vertexClip[0];\n"
                                            "   fragColor = vertexColor[0];\n"
                                            "   EmitVertex();\n"
                                            "   EndPrimitive();\n"
                                            "}"));


      opaque->addFragmentShaderCode(QString("centroid in vec4 fragColor;\n"
                                            "void main()\n"
                                            "{\n"
                                            "  gl_FragColor = fragColor;\n"
                                            "}\n"));
      if(not opaque->setupShaders())
        out << "Warning, cannot compile Edge opaque shaders" << endl;

      trans->changeVertexShaderCode(Drawable::VS_CLIPPING,
                                     QString("in vec3 center;\n"
                                             "out vec4 vertexClip;\n"
                                             "void clipVertex(vec4 pos)\n"
                                             "{\n"
                                             "  vertexClip = gl_ModelViewMatrix*vec4(center,1);\n"
                                             "}\n"));

      trans->setupShaders();
    }

    void finalize(Viewer *)
    {
      opaque->cleanShaders();
      delete opaque;
      opaque = 0;
      trans->cleanShaders();
      delete trans;
      trans = 0;
    }

    void drawOpaque(Viewer *)
    {
      if(showNormals)
      {
        opaque->useShaders();
        opaque->setUniform_instant("normalSize", drawing::GLSLValue(normalSize));
        GLint loc = opaque->attribLocation("vnormal");

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableVertexAttribArray(loc);

        glColor3fv(normalColor.c_data());

        glVertexAttribPointer(loc, 3, GL_DOUBLE, GL_FALSE, 0, &faces_normals.front());
        glLineWidth(normalThickness);
        glVertexPointer(3, GL_DOUBLE, 0, &center_faces.front());
        glDrawArrays(GL_POINTS, 0, center_faces.size());

        glDisableVertexAttribArray(loc);
        glDisableClientState(GL_VERTEX_ARRAY);
        opaque->stopUsingShaders();
      }
    }

    void drawTransparent(Viewer *v)
    {
      QMutexLocker lock(this->mutex());
      glDisable(GL_CULL_FACE);
      //out << "Faces::drawOpaque() with " << this->data->faces.size() << " elements" << endl;
      this->trans->setUniform("two_sided_light", drawing::GLSLValue(true));
      this->trans->setUniform("nbLights", drawing::GLSLValue(v->nbLights()));
      this->trans->useShaders();
      this->trans->setUniform_instant("transparency", drawing::GLSLValue(faceTransparency));
      glColor3f(1,1,1);

      GLint loc_color = this->trans->attribLocation("vertexColorIdx");
      GLint loc_center = this->trans->attribLocation("center");
      glEnableVertexAttribArray(loc_color);
      glEnableVertexAttribArray(loc_center);

      Shader::activeTexture(CDAT_PALETTE);
      glBindTexture(GL_TEXTURE_1D, this->palTexId);
      Shader::activeTexture(AT_NONE);
      glBindTexture(GL_TEXTURE_1D, 0);

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_NORMAL_ARRAY);

      glVertexAttribPointer(loc_color, 1, GL_FLOAT, GL_FALSE, 0, &colors.front());
      glVertexAttribPointer(loc_center, 3, GL_DOUBLE, GL_FALSE, 0, &faces_center.front());
      glNormalPointer(GL_DOUBLE, 0, &normals.front());
      glVertexPointer(3, GL_DOUBLE, 0, &faces.front());

      glDrawArrays(GL_TRIANGLES, 0, faces.size());

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_NORMAL_ARRAY);
      glDisableVertexAttribArray(loc_color);
      glDisableVertexAttribArray(loc_center);

      Shader::activeTexture(CDAT_PALETTE);
      glBindTexture(GL_TEXTURE_1D, 0);
      Shader::activeTexture(AT_NONE);

      this->trans->stopUsingShaders();

      this->trans->setUniform("two_sided_light", drawing::GLSLValue(false));
      REPORT_GL_ERROR("Faces::drawTransparent()");
    }

    /*
    QWidget *controls()
    {
      QUiLoader l;
      QFile f("faces.ui");
      if(!f.open(QIODevice::ReadOnly))
      {
        out << "Error opening faces.ui" << endl;
        return 0;
      }
      QWidget *w = l.load(&f);
      QSpinBox *tr = w->findChild<QSpinBox*>("transparencyFace");
      if(!tr)
      {
        out << "Error, cannot find spin box 'transparencyFace'" << endl;
        delete w;
        return 0;
      }
      QDoubleSpinBox *nt = w->findChild<QDoubleSpinBox*>("normalThickness");
      if(!nt)
      {
        out << "Error, cannot find double spin box 'normalThickness'" << endl;
        delete w;
        return 0;
      }
      QDoubleSpinBox *ns = w->findChild<QDoubleSpinBox*>("normalSize");
      if(!ns)
      {
        out << "Error, cannot find double spin box 'normalSize'" << endl;
        delete w;
        return 0;
      }
      QGroupBox *sn = w->findChild<QGroupBox*>("showNormals");
      if(!sn)
      {
        out << "Error, cannot find group box 'showNormals'" << endl;
        delete w;
        return 0;
      }
      GLfloat lw[2];
      glGetFloatv(GL_LINE_WIDTH_RANGE, lw);
      nt->setMinimum(lw[0]);
      nt->setMaximum(lw[1]);
      nt->setSingleStep(1.0);

      ns->setValue(normalSize);
      nt->setValue(normalThickness);
      tr->setValue(int(100*faceTransparency));

      sn->setChecked(showNormals);

      connect(tr, SIGNAL(valueChanged(int)), this, SLOT(changeTransparency(int)));
      connect(nt, SIGNAL(valueChanged(double)), this, SLOT(changeThickness(double)));
      connect(ns, SIGNAL(valueChanged(double)), this, SLOT(changeNormalSize(double)));
      connect(sn, SIGNAL(toggled(bool)), this, SLOT(changeShowNormal(bool)));

      return w;
    }
    */

  public slots:

  void changeNormalSize(double v)
  {
    normalSize = float(v);
    this->redraw();
  }

  void changeThickness(double v)
  {
    normalThickness = float(v);
    this->redraw();
  }

  void changeTransparency(int t)
  {
    faceTransparency = float(t)/100;
    this->redraw();
  }

  void changeShowNormal(bool on)
  {
    showNormals = on;
    this->redraw();
  }
};

class Cells : public GenCell
{
  Q_OBJECT
  public:
  std::vector<Point3d> cells, normals, centers;
  std::vector<float> colors;
  std::vector<Point3d> cell_centers;
  float cellTransparency, cellShift;
  bool showCenter;
  double centerRadius;
  Shader *opaque;

  Cells(QMutex *m, QObject *parent)
    : GenCell(m, parent)
    , cellTransparency(0.7)
  {
    this->setObjectName("Cells");
  }

  void init(Viewer *, Shader *t, Shader *def)
  {
    trans = new Shader(*t);
    trans->changeVertexShaderCode(Drawable::VS_POSITION,
                                  QString("in vec3 center;\n"
                                          "uniform float cellShift;\n"
                                          "vec4 position() {\n"
                                          "  if(gl_Vertex.w == 0) return gl_Vertex;\n"
                                          "  else {\n"
                                          "    vec3 pos = gl_Vertex.xyz / gl_Vertex.w;\n"
                                          "    pos = cellShift*center + (1-cellShift)*pos;\n"
                                          "    return vec4(pos,1.0);\n"
                                          "  }\n"
                                          "}\n")
                                 );
    trans->changeVertexShaderCode(Drawable::VS_COLOR,
                                  QString("in float vertexColorIdx;\n"
                                          "uniform sampler1D palette;\n"
                                          "out vec4 vertexColor;\n"
                                          "void color()\n"
                                          "{\n"
                                          "  vertexColor = texture1D(palette, vertexColorIdx);\n"
                                          "}\n")
                                  );
    trans->changeVertexShaderCode(Drawable::VS_CLIPPING,
                                  QString("out vec4 vertexClip;\n"
                                          "void clipVertex(vec4 pos)\n"
                                          "{\n"
                                          "  vertexClip = gl_ModelViewMatrix*vec4(center,1);\n"
                                          "}\n"));

    trans->setupShaders();
    trans->setUniform("palette", drawing::GLSLValue(CDAT_PALETTE));

    opaque = new Shader(*def);
    opaque->changeVertexShaderCode(Drawable::VS_CLIPPING,
                                   QString("out vec4 vertexClip;\n"
                                           "in vec3 center;\n"
                                           "void clipVertex(vec4 pos)\n"
                                           "{\n"
                                           "  vertexClip = gl_ModelViewMatrix*vec4(center,1);\n"
                                           "}\n"));
    opaque->setupShaders();
    opaque->setUniform("peeling", drawing::GLSLValue(false));
  }

  void finalize(Viewer *)
  {
    if(trans) {
      trans->cleanShaders();
      delete trans;
      trans = 0;
    }
    if(opaque) {
      opaque->cleanShaders();
      delete opaque;
      opaque = 0;
    }
  }

  void drawOpaque(Viewer *v)
  {
    if(showCenter)
    {
      opaque->setUniform("nbLights", drawing::GLSLValue(v->nbLights()));
      opaque->useShaders();
      glColor3f(1,0,0);
      for(const Point3d& cp: cell_centers)
        shape::sphere(cp, centerRadius, 10, 10);
      glColor3f(1,1,1);
      opaque->stopUsingShaders();
    }
  }

  void drawTransparent(Viewer *v)
  {
    QMutexLocker lock(this->mutex());
    //out << "Cells::drawTransparent()" << endl;

    glEnable(GL_CULL_FACE);
    trans->setUniform("nbLights", drawing::GLSLValue(v->nbLights()));

    glColor3f(1,1,1);
    trans->useShaders();
    trans->setUniform_instant("transparency", drawing::GLSLValue(cellTransparency));
    trans->setUniform_instant("cellShift", drawing::GLSLValue(cellShift));

    Shader::activeTexture(CDAT_PALETTE);
    glBindTexture(GL_TEXTURE_1D, palTexId);
    Shader::activeTexture(AT_NONE);
    glBindTexture(GL_TEXTURE_1D, 0);

    GLint loc_color = trans->attribLocation("vertexColorIdx");
    GLint loc_center = trans->attribLocation("center");
    if(loc_color < 0) out << "Error get vertexColorIdx location: " << loc_color << endl;
    if(loc_center < 0) out << "Error get center location: " << loc_center << endl;

    glEnableVertexAttribArray(loc_color);
    glEnableVertexAttribArray(loc_center);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexAttribPointer(loc_center, 3, GL_DOUBLE, GL_FALSE, 0, &centers.front());
    glVertexAttribPointer(loc_color, 1, GL_FLOAT, GL_FALSE, 0, &colors.front());
    glVertexPointer(3, GL_DOUBLE, 0, &cells.front());
    glNormalPointer(GL_DOUBLE, 0, &normals.front());

    glDrawArrays(GL_TRIANGLES, 0, cells.size());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableVertexAttribArray(loc_center);
    glDisableVertexAttribArray(loc_color);

    Shader::activeTexture(CDAT_PALETTE);
    glBindTexture(GL_TEXTURE_1D, 0);
    Shader::activeTexture(AT_NONE);

    this->trans->stopUsingShaders();
    REPORT_GL_ERROR("Cells::drawTransparent()");
  }

  /*
  QWidget *controls()
  {
    QUiLoader l;
    QFile f("cells.ui");
    if(!f.open(QIODevice::ReadOnly))
    {
      out << "Error opening cells.ui" << endl;
      return 0;
    }
    QWidget *w = l.load(&f);
    QSpinBox *tr = w->findChild<QSpinBox*>("transparencyCell");
    if(!tr)
    {
      out << "Error, cannot find spin box 'transparencyCell'" << endl;
      delete w;
      return 0;
    }
    QSpinBox *cs = w->findChild<QSpinBox*>("cellShift");
    if(!cs)
    {
      out << "Error, cannot find spin box 'cellShift'" << endl;
      delete w;
      return 0;
    }
    QGroupBox *cc = w->findChild<QGroupBox*>("cellCenter");
    if(!cc)
    {
      out << "Error, cannot find group box 'cellCenter'" << endl;
      delete w;
      return 0;
    }
    QDoubleSpinBox *cr = w->findChild<QDoubleSpinBox*>("centerRadius");
    if(!cr)
    {
      out << "Error, cannot find spin box 'centerRadius'" << endl;
      delete w;
      return 0;
    }

    tr->setValue(int(100.0*cellTransparency));
    cs->setValue(int(100.0*(1.0-cellShift)));
    cr->setValue(centerRadius);
    cc->setChecked(showCenter);

    connect(tr, SIGNAL(valueChanged(int)), this, SLOT(changeTransparency(int)));
    connect(cs, SIGNAL(valueChanged(int)), this, SLOT(changeCellShift(int)));
    connect(cr, SIGNAL(valueChanged(double)), this, SLOT(changeCellRadius(double)));
    connect(cc, SIGNAL(toggled(bool)), this, SLOT(showCellCenter(bool)));

    return w;
  }
  */

  public slots:

  void changeTransparency(int t)
  {
    cellTransparency = float(t)/100.0;
    this->redraw();
  }

  void changeCellShift(int t)
  {
    cellShift = 1.0-float(t)/100.0;
    this->redraw();
  }

  void showCellCenter(bool show)
  {
    showCenter = show;
    this->redraw();
  }

  void changeCellRadius(double r)
  {
    centerRadius = r;
    this->redraw();
  }

};


template <typename Model, typename Tissue>
struct ComplexDrawer : public Drawable
{
  typedef typename Tissue::cell_t cell;
  typedef typename Tissue::face_t face;
  typedef typename Tissue::edge_t edge;
  typedef typename Tissue::vertex_t vertex;
  typedef typename Tissue::oriented_face_t oriented_face;

  ComplexDrawer(Model* model, Tissue& t, Palette& p)
    : Drawable(model)
    , trans(0)
    , m(QMutex::Recursive)
    , vs(0)
    , es(0)
    , fs(0)
    , cs(0)
    , model(model)
    , T(&t)
    , palette(&p)
    , palTexId(0)
    , palette_updated(false)
  {
    palette_texture.resize(256);

    vs = new Vertices(mutex(), this);
    es = new Edges(mutex(), this);
    fs = new Faces(mutex(), this);
    cs = new Cells(mutex(), this);

    this->setObjectName("Cell Complex");
    addFilename("view.v");
    addFilename("pal.map");

    reread();
  }

  ~ComplexDrawer()
  {
    if(trans) delete trans;
    trans = 0;
  }

  void init(Viewer *v, Shader *s)
  {
    trans = new drawing::Shader(*s);
    //trans->setVerbosity(5);
    trans->changeVertexShaderCode(Drawable::VS_COLOR,
                                  QString("in float vertexColorIdx;\n"
                                          "uniform sampler1D palette;\n"
                                          "out vec4 vertexColor;\n"
                                          "void color() {\n"
                                          "  vertexColor = texture1D(palette, vertexColorIdx);\n"
                                          "}\n")
                                  );
    trans->changeGeometryShaderCode(Drawable::GS_COLOR,
                                    QString("in vec4 vertexColor[];\n"
                                            "flat out vec4 faceColor;\n"
                                            "void color(int idx) {\n"
                                            "  faceColor = vertexColor[idx];\n"
                                            "}\n"));
    trans->changeFragmentShaderCode(Drawable::FS_COLOR,
                                    QString("uniform float transparency;\n"
                                            "flat in vec4 faceColor;\n"
                                            "void color() {\n"
                                            "  if(transparency > 0) {\n"
                                            "    gl_FragColor = light(faceColor);\n"
                                            "    gl_FragColor.a = transparency;\n"
                                            "  } else discard;\n"
                                            "}\n"
                                           ));
    trans->setupShaders();

    // Allocate palette texture
    glGenTextures(1, &palTexId);
    glBindTexture(GL_TEXTURE_1D, palTexId);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, &palette_texture.front());
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_1D, 0);

    palette_updated = true;

    trans->setUniform("palette", drawing::GLSLValue(CDAT_PALETTE));

    vs->setPalTexId(palTexId);
    vs->init(v, trans, s);
    es->setPalTexId(palTexId);
    es->init(v, trans, s);
    fs->setPalTexId(palTexId);
    fs->init(v, trans, s);
    cs->setPalTexId(palTexId);
    cs->init(v, trans, s);
  }

  void finalize(Viewer *)
  {
    if(palTexId)
    {
      glDeleteTextures(1, &palTexId);
      palTexId = 0;
    }
    if(trans)
    {
      trans->cleanShaders();
      delete trans;
      trans = 0;
    }
  }

  void preDraw(Viewer *)
  {
    QMutexLocker lock(mutex());
    if(palette_updated)
    {
      palette_updated = false;
      glBindTexture(GL_TEXTURE_1D, palTexId);
      glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, &palette_texture.front());
      glBindTexture(GL_TEXTURE_1D, 0);
    }
  }

  void updatePositions()
  {
    QMutexLocker lock(mutex());
  }

  void updateColors()
  {
    QMutexLocker lock(mutex());
  }

  Point3d vertexNormal(vertex v, face f, cell c) const
  {
    if(!c) c = T->anyCoface(f);
    if(!c) return f->normal;
    QueryType Q;
    Point3d normal = int(T->relativeOrientation(c,f))*model->normal(f)*model->area(f);
    auto pcfs = T->match(f,Q,Q,v);
    vvassert(not pcfs.empty());
    auto pcf = *pcfs.begin();
    edge e1 = pcf->face1;
    edge e2 = pcf->face2;
    face pf = f;
    while(!model->crease(e1) and e1 != e2)
    {
      face nf = T->flip(c,pf,e1);
      edge ne1 = T->flip(nf,e1,v);
      Point3d n = int(T->relativeOrientation(c,nf))*nf->normal * nf->area;
      normal += n;
      pf = nf;
      e1 = ne1;
    }
    if(e1 != e2)
    {
      pf = f;
      while(!model->crease(e2))
      {
        face nf = T->flip(c,pf,e2);
        edge ne1 = T->flip(nf,e2,v);
        Point3d n = int(T->relativeOrientation(c,nf))*nf->normal * nf->area;
        normal += n;
        pf = nf;
        e2 = ne1;
      }
    }
    return util::normalized(normal);
  }

  void addFaceTriangles(const float& col,
                        size_t& num, std::vector<Point3d>& fs,
                        std::vector<Point3d>& ns,
                        std::vector<float>& cols,
                        const face& f,
                        const cell& c = cell::null) const
  {
    Point3d cpos;
    RelativeOrientation orien = pos;
    if(c)
    {
      orien = T->relativeOrientation(c,f);
      cpos = model->position(c);
    }
    std::vector<vertex> vertices = T->orderedVertices(orien*f);
    std::vector<Point3d> vs(vertices.size()), normals(vertices.size());
    Point3d n;
    for(size_t i = 0 ; i < vertices.size() ; i++)
    {
      vertex v = vertices[i];
      vs[i] = model->position(v);
      if(c)
        normals[i] = vertexNormal(v, f, c);
      else
        normals[i] = f->normal;
      n += normals[i];
    }

    Point3d center = model->position(f);
    normalize(n);
    size_t prev = vs.size()-1;
    for(size_t i = 0 ; i < vs.size() ; ++i)
    {
      ns[num] = n;
      cols[num] = col;
      fs[num++] = center;
      ns[num] = normals[prev];
      cols[num] = col;
      fs[num++] = vs[prev];
      ns[num] = normals[i];
      cols[num] = col;
      fs[num++] = vs[i];
      prev = i;
    }
  }

  void updateGeometry()
  {
    QMutexLocker lock(mutex());

    std::vector<Point3d>& pts = vs->pts;

    pts.resize(T->nbVertices());

    size_t num = 0;
    pmin = Point3d(HUGE_VAL,HUGE_VAL,HUGE_VAL);
    pmax = -pmin;
    for(const vertex& v: T->vertices())
    {
      pts[num++] = model->position(v);
      const Point3d& vpos = model->position(v);
      if(vpos.x() < pmin.x()) pmin.x() = vpos.x();
      if(vpos.y() < pmin.y()) pmin.y() = vpos.y();
      if(vpos.z() < pmin.z()) pmin.z() = vpos.z();
      if(vpos.x() > pmax.x()) pmax.x() = vpos.x();
      if(vpos.y() > pmax.y()) pmax.y() = vpos.y();
      if(vpos.z() > pmax.z()) pmax.z() = vpos.z();
    }

    std::vector<Point3d>& lines = es->lines;
    std::vector<Point3d>& lines_center = es->centers;

    lines.resize(T->nbEdges()*2);
    lines_center.resize(T->nbEdges()*2);
    num = 0;
    for(const edge& e: T->edges())
    {
      vertex v1(0), v2(0);
      std::tie(v1,v2) = T->orderedVertices(+e);
      lines_center[num] = lines_center[num+1] = (v1->pos+v2->pos)/2;
      lines[num++] = v1->pos;
      lines[num++] = v2->pos;
    }
    vvassert(num == lines.size());

    // Count the number of triangles needed for the faces and cells
    size_t nb_faces = 0;
    size_t nb_cells = 0;
    for(const face& f: T->faces())
    {
      size_t nbf = T->nbFaces(f);
      nb_faces += nbf;
      nb_cells += nbf * T->nbCofaces(f);
    }
    nb_faces *= 3; // 3 pts per triangle
    nb_cells *= 3;

    std::vector<Point3d>& faces = fs->faces;
    std::vector<Point3d>& faces_normals = fs->normals;
    std::vector<float>& faces_color = fs->colors;
    std::vector<Point3d>& faces_center = fs->faces_center;


    faces.resize(nb_faces);
    faces_normals.resize(nb_faces);
    faces_color.resize(nb_faces);
    faces_center.resize(nb_faces);

    out << "Finding faces" << endl;
    num = 0;
    {
      size_t old_num = num;
      for(const face& f: T->faces())
      {
        float color = 68.5f / 256.0f;
        addFaceTriangles(color, num, faces, faces_normals, faces_color, f);
        for( ; old_num < num ; ++old_num)
          faces_center[old_num] = model->position(f);
      }
    }
    vvassert(num == faces.size());
    vvassert(num == faces_normals.size());

    std::vector<Point3d>& cells = cs->cells;
    std::vector<Point3d>& cells_normals = cs->normals;
    std::vector<Point3d>& cells_center = cs->centers;
    std::vector<float>& cells_color = cs->colors;
    std::vector<Point3d>& cells_center_pos = cs->cell_centers;

    cells.resize(nb_cells);
    cells_normals.resize(nb_cells);
    cells_color.resize(nb_cells);
    cells_center.resize(nb_cells);
    cells_center_pos.resize(T->nbCells());

    out << "Finding cells" << endl;
    num = 0;
    size_t n_cell = 0;
    for(const cell& c: T->cells())
    {
      float col = (48.5f+float(model->cellIndex(c) % 8))/256.0f;
      size_t old_num = num;
      for(const face& f: T->faces(c))
      {
        addFaceTriangles(col, num, cells, cells_normals, cells_color, f, c);
      }
      for( ; old_num < num ; ++old_num)
        cells_center[old_num] = model->position(c);
      cells_center_pos[n_cell++] = model->position(c);
    }
    vvassert(num == cells.size());
    vvassert(num == cells_normals.size());

    out << "\n# vertices = " << pts.size()
        << "\n# edges    = " << lines.size()
        << "\n# faces    = " << faces.size()
        << "\n# cells    = " << cells.size() << endl;

    {
      std::vector<Point3d>& normals = fs->faces_normals;
      std::vector<Point3d>& centers = fs->center_faces;
      normals.resize(T->nbFaces());
      centers.resize(T->nbFaces());
      num = 0;
      for(const face& f: T->faces())
      {
        normals[num] = f->normal;
        centers[num++] = model->position(f);
      }
      vvassert(num == normals.size());
    }

    /*
     *{
     *  std::vector<Point3d>& normals = vs->normals;
     *  std::vector<Point3d>& starts = vs->starts;
     *  starts.resize(2*T->nbVertices());
     *  normals.resize(2*T->nbVertices());
     *  num = 0;
     *  for(const vertex& v: T->vertices())
     *  {
     *    if(!v->corner)
     *    {
     *      normals[num] = Point3d();
     *      starts[num++] = model->position(v);
     *      normals[num] = v->normal;
     *      starts[num++] = model->position(v);
     *    }
     *  }
     *  vvassert(num <= normals.size());
     *  normals.resize(num);
     *  starts.resize(num);
     *}
     */

  }

  BSphere boundingSphere() const
  {
    return BSphere((pmin+pmax)/2, util::norm(pmax-pmin)/2);
  }

  void setCellComplex(Tissue *T_)
  {
    T_ = T_;
  }

  void reread()
  {
    QMutexLocker lock(mutex());
    util::Parms parms("view.v");

    parms("ComplexDrawer", "CellShift", cs->cellShift);
    float ns;
    parms("ComplexDrawer", "NormalSize", ns);
    parms("ComplexDrawer", "CellCenterSize", cs->centerRadius);
    int vertexColor;
    parms("ComplexDrawer", "VertexColor", vertexColor);
    int edgeColor;
    parms("ComplexDrawer", "EdgeColor", edgeColor);
    int faceNormalColor;
    parms("ComplexDrawer", "FaceNormalColor", faceNormalColor);
    fs->normalSize = ns;

    vs->color = palette->getColor(vertexColor);
    es->color = palette->getColor(edgeColor);
    fs->normalColor = palette->getColor(faceNormalColor);
    palette_updated = true;

    for(int i = 0 ; i < 256 ; ++i)
      palette_texture[i] = palette->getColor(i);

    redraw();
  }

  QMutex* mutex()
  {
    return &m;
  }

  Shader *trans;
  QMutex m;

  Vertices *vs;
  Edges *es;
  Faces *fs;
  Cells *cs;

  const Model* model;
  const Tissue* T;
  const Palette* palette;

  bool showFaceNormals, showVertexNormals;
  std::vector<Color> palette_texture;
  GLuint palTexId;
  bool palette_updated;

  Point3d pmin, pmax;
};

#include "complex_drawer.moc"

#endif // DRAWER_VVH

