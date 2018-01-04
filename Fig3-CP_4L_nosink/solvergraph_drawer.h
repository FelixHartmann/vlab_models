#ifndef SOLVERGRAPH_DRAWER_H
#define SOLVERGRAPH_DRAWER_H

#include <drawing/drawable.h>
#include <util/palette.h>
#include <util/assert.h>

#include <QMutex>
#include <QMutexLocker>

#include <limits>

#include "structure.h"

using cellflips::out;
using util::Palette;
typedef util::Palette::Color Colorf;

namespace std {

template <typename T1, typename T2>
struct hash<std::pair<T1,T2>>
{
  static const std::hash<T1> h1;
  static const std::hash<T2> h2;
  size_t operator()(const std::pair<T1,T2>& t) const
  {
    return h1(t.first) ^ h2(t.second);
  }
};

template <typename T1, typename T2>
const std::hash<T1> hash<std::pair<T1,T2>>::h1;

template <typename T1, typename T2>
const std::hash<T2> hash<std::pair<T1,T2>>::h2;
} // namespace std

template <typename Model>
class DrawSolverGraph : public drawing::Drawable
{
public:
  DrawSolverGraph(Model *model_, const Tissue& T_, const SolverGraph& S_, Palette& p)
    : drawing::Drawable(model_)
    , model(model_)
    , T(T_)
    , S(S_)
    , palette(&p)
    , _data(model_, T_, S_, p)
    , _new_data(model_, T_, S_, p)
  {
    setObjectName("Solver Graph");
    setCanMove(false);
    setFilename("view.v");
    reread();
  }

  void updateGeometry()
  {
    out << "DrawSolverGraph::updateGeometry" << endl;
    if(not S.empty()) {
      _bsphere = _new_data.set_positions_and_colors(colorCell, colorMembrane, colorApoplast, colorLink);
    } else {
      _bsphere = BSphere({0,0,0}, 1.);
      out << "    ... without cells" << endl;
    }

    using std::swap;

    {
      QMutexLocker updating(&_lock);
      _data.swap(_new_data);
      _new_data.clear();
    }

    _new_data.clear();
  }

  void reread() override
  {
    util::Parms parms(QString::fromStdString(filename()));

    parms("SolverGraphDrawer", "ColorCell", colorCell);
    parms("SolverGraphDrawer", "ColorMembrane", colorMembrane);
    parms("SolverGraphDrawer", "ColorApoplast", colorApoplast);
    parms("SolverGraphDrawer", "ColorLink", colorLink);
    parms("SolverGraphDrawer", "LinkThickness", linkThickness);
    parms("SolverGraphDrawer", "SphereSize", sphereSize);

    updateGeometry();
  }

  void drawOpaque(Viewer *) override
  {
    drawArrays();
  }

  BSphere boundingSphere() const override
  {
    return _bsphere;
  }

protected:
  void drawArrays()
  {
    QMutexLocker drawing(&_lock);

    glDisable(GL_LIGHTING);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glColorPointer(4, GL_FLOAT, 0, _data.node_colors.data());
    glVertexPointer(3, GL_DOUBLE, 0, _data.node_positions.data());
    glDrawElements(GL_LINES, _data.line_positions.size(), GL_UNSIGNED_INT, _data.line_positions.data());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_LIGHTING);
  }

  Model *model;
  const Tissue& T;
  const SolverGraph& S;
  const Palette* palette;

  float sphereSize, linkThickness;
  int colorCell, colorMembrane, colorApoplast, colorLink;

  struct RenderingData {
    RenderingData(Model* model_, const Tissue& T_, const SolverGraph& S_, Palette& p)
      : model(model_)
      , T(T_)
      , S(S_)
      , palette(&p)
    { }

    void clear() {
      node_positions.clear();
      node_colors.clear();
      line_positions.clear();
    }

    void swap(RenderingData& other) {
      using std::swap;
      swap(node_positions, other.node_positions);
      swap(node_colors, other.node_colors);
      swap(line_positions, other.line_positions);
    }

    BSphere set_positions_and_colors(int colorCell, int colorMembrane, int colorApoplast, int colorLink)
    {
      std::cout << "Entering set_positions_and_colors()." << std::endl;
      Point3d pmin(HUGE_VAL), pmax(-HUGE_VAL);
      size_t nb_edges = 0;

      // Compute the number of edges.
      for (const node& n: S) {
        nb_edges += S.valence(n);
      }
      nb_edges /= 2;

      size_t nb_graph_nodes = S.size();
      size_t nb_nodes_to_draw = nb_graph_nodes + nb_edges;

      node_positions.resize(nb_nodes_to_draw);
      node_colors.resize(nb_nodes_to_draw);
      line_positions.resize(4*nb_edges);

      std::unordered_map<node, Point3d> pos_map;
      std::unordered_map<node, GLuint> idx_map;
      std::unordered_map<std::pair<GLuint,GLuint>, GLuint> midpoint_map;

      // Position, index and color of the graph nodes.
      GLuint i = 0;
      for (const node& n: S) {
        Point3d pos;
        Colorf node_color;
        switch(n->type)
        {
          case NT_CELL:
            {
              CellLink *cl = static_cast<CellLink*>(n->link);
              pos = cl->cel->pos;
              node_color = palette->getColor(colorCell);
            }
            break;
          case NT_MEMBRANE:
            {
              // positionned 2 times nearer from the cell it belongs than from
              // the adjacent cell
              MembraneLink *ml = static_cast<MembraneLink*>(n->link);
              oriented_face of = ml->membrane;
              auto c1 = cell::null;
              for (const node& nn: S.neighbors(n))
              {
                if (nn->type == NT_CELL) {
                  CellLink *cl = static_cast<CellLink*>(nn->link);
                  c1 = cl->cel;
                }
              }
              cell c2 = T.flip(T.T, c1, ~of);
              vvassert(c1 and c2);
              pos = (3*c1->pos + c2->pos)/4;
              node_color = palette->getColor(colorMembrane);
            }
            break;
          case NT_APOPLAST:
            {
              // positionned halfway the two cells
              ApoplastLink *al = static_cast<ApoplastLink*>(n->link);
              face apoplast = al->apoplast;
              for (const cell& cn: T.cofaces(apoplast))
              {
                pos += cn->pos;
              }
              pos /= 2;
              node_color = palette->getColor(colorApoplast);
            }
            break;
        }
        if(pos.x() < pmin.x()) pmin.x() = pos.x();
        if(pos.y() < pmin.y()) pmin.y() = pos.y();
        if(pos.z() < pmin.z()) pmin.z() = pos.z();
        if(pos.x() > pmax.x()) pmax.x() = pos.x();
        if(pos.y() > pmax.y()) pmax.y() = pos.y();
        if(pos.z() > pmax.z()) pmax.z() = pos.z();
        pos_map[n] = pos;
        idx_map[n] = i;
        node_positions[i] = pos;
        node_colors[i] = node_color;
        i++;
      }

      // Add midpoints.
      for (const node& n: S) {
        GLuint idx1;
        try {
          idx1 = idx_map.at(n);
        } catch(std::out_of_range e) {
          out << "Error, missing node in index map." << endl;
          throw;
        }
        for (const node& nn: S.neighbors(n)) {
          GLuint idx2;
          try {
            idx2 = idx_map.at(nn);
          } catch(std::out_of_range e) {
            out << "Error, missing node in index map." << endl;
            throw;
          }
          if (n < nn) {
            Point3d pos = (pos_map[n] + pos_map[nn])/2;
            node_positions[i] = pos;
            node_colors[i] = palette->getColor(colorLink);
            std::pair<GLuint, GLuint> idx_pair;
            if (idx1 < idx2)
              idx_pair = std::make_pair(idx1, idx2);
            else
              idx_pair = std::make_pair(idx2, idx1);
            midpoint_map[idx_pair] = i;
            i++;
          }
        }
      }

      vvassert(i == node_positions.size());

      // Lines positions.
      int j = 0;
      for (const node& n: S) {
        GLuint idx1 = idx_map[n];
        for (const node& nn: S.neighbors(n)) {
          GLuint idx2 = idx_map[nn];
          GLuint idx_mid;
          if (idx1 < idx2)
            idx_mid = midpoint_map.at(std::make_pair(idx1, idx2));
          else
            idx_mid = midpoint_map.at(std::make_pair(idx2, idx1));
          line_positions[j] = idx1;
          j++;
          line_positions[j] = idx_mid;
          j++;
        } 
      }

      vvassert(j == line_positions.size());

      return BSphere((pmin+pmax)/2, util::norm(pmax-pmin)/2);
    }

    Model* model;
    const Tissue& T;
    const SolverGraph& S;
    const Palette* palette;

    std::vector<Point3d> node_positions;
    std::vector<Colorf> node_colors;
    std::vector<GLuint> line_positions;

  } _data, _new_data;

  BSphere _bsphere;

  QMutex _lock;
};

#endif // SOLVERGRAPH_DRAWER_H

