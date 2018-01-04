
#line 1 "structure.vvh"
#ifndef STRUCTURE_VVH
#define STRUCTURE_VVH

#include <geometry/geometry.h>
#include <cstdint>
#include <cellflips/cellflips.h>
#include <algorithms/solver.h>

using geometry::Point3d;

typedef util::Vector<5,double> Point5d;
typedef solver::Solver<5> RDSolver;
typedef RDSolver::tag_t rd_tag_t;

enum CellType
{
  CORPUS = 0,
  L1,
  SOURCE,
  SINK
};

enum Chemical
{
  AUXIN = 0,
  PIN,
  APIN,
  AAUX,
  VAF
};

enum NodeType
{
  NT_CELL = 0,
  NT_MEMBRANE = 1,
  NT_APOPLAST = 2
};

 

#line 40 "structure.vvh"


  struct p975758e2_f14b_11e7_aac5_3417eba08742_cell_content {
    typedef p975758e2_f14b_11e7_aac5_3417eba08742_cell_content Self;

#line 42 "structure.vvh"

    bool is_anchor = false;
    CellType type = CORPUS;

    Point3d pos;
    Point3d circumcenter;
    double area;
    double volume;

    double auxin = 0;
    double PIN = 0;
    double dauxin = 0;
    double dPIN = 0;

    bool PIN_excess = false;  // if excessive amount of PIN
  };

#line 57 "structure.vvh"



  struct p975758e2_f14b_11e7_aac5_3417eba08742_face_content {
    typedef p975758e2_f14b_11e7_aac5_3417eba08742_face_content Self;

#line 60 "structure.vvh"

    bool is_anchor = false;

    Point3d pos, normal;
    double volume;
    double area;

    double auxin = 0;
    double dauxin = 0;
    double VAF = 0;
    double dVAF = 0;

    double PINpos = 0;
    double dPINpos = 0;
    double APINpos = 0;
    double dAPINpos = 0;
    double AAUXpos = 0;
    double dAAUXpos = 0;
    double VAFpos = 0;
    double dVAFpos = 0;

    double PINneg = 0;
    double dPINneg = 0;
    double APINneg = 0;
    double dAPINneg = 0;
    double AAUXneg = 0;
    double dAAUXneg = 0;
    double VAFneg = 0;
    double dVAFneg = 0;
  };

#line 89 "structure.vvh"



  struct p975758e2_f14b_11e7_aac5_3417eba08742_edge_content {
    typedef p975758e2_f14b_11e7_aac5_3417eba08742_edge_content Self;

#line 92 "structure.vvh"

    bool is_anchor = false;
    double length;
    double area;
  };

#line 96 "structure.vvh"


  
  struct p975758e2_f14b_11e7_aac5_3417eba08742_vertex_content {
    typedef p975758e2_f14b_11e7_aac5_3417eba08742_vertex_content Self;

#line 99 "structure.vvh"

    bool is_anchor = false;
    // The following `type' member can be used in a Delaunay complex
    // in order to attribute a type to the corresponding in the dual
    // Voronoi complex.
    CellType type = CORPUS;
    size_t id;
    Point3d pos;
  };

#line 107 "structure.vvh"

typedef cellflips::CellComplex<p975758e2_f14b_11e7_aac5_3417eba08742_vertex_content, p975758e2_f14b_11e7_aac5_3417eba08742_edge_content, p975758e2_f14b_11e7_aac5_3417eba08742_face_content, p975758e2_f14b_11e7_aac5_3417eba08742_cell_content> Tissue;
typedef Tissue::cell_t cell;
typedef Tissue::edge_t edge;
typedef Tissue::vertex_t ccvertex;
typedef Tissue::face_t face;
typedef Tissue::oriented_face_t oriented_face;
typedef Tissue::oriented_vertex_t oriented_vertex;
typedef Tissue::oriented_cell_t oriented_cell;
typedef Tissue::oriented_edge_t oriented_edge;

#line 108 "structure.vvh"


// Class linking a node in the solver graph to either a cell, a membrane (i.e.
// a wall in a cell) or a cell wall (i.e. a wall_arc)
struct SolverLink
{
  SolverLink(NodeType t = NT_CELL)
    : type(t)
  {}
  virtual ~SolverLink() {}
  NodeType type;
  virtual void setChems(const Point5d& c, const Point5d& dc) = 0;
  virtual void update(Point5d& c, Point5d& dc) = 0;
};

struct CellLink : public SolverLink
{
  cell cel;

  CellLink(const cell& c)
    : SolverLink(NT_CELL)
    , cel(c)
  {}

  void setChems(const Point5d& c, const Point5d& dc)
  {
    cel->auxin = c[AUXIN];
    cel->PIN = c[PIN];
    cel->dauxin = dc[AUXIN];
    cel->dPIN = dc[PIN];
  }

  void update(Point5d& c, Point5d& dc)
  {
    c[AUXIN] = cel->auxin;
    c[PIN] = cel->PIN;
    dc[AUXIN] = cel->dauxin;
    dc[PIN] = cel->dPIN;
  }

};

struct MembraneLink : public SolverLink
{
  oriented_face membrane;

  MembraneLink(const oriented_face& of)
    : SolverLink(NT_MEMBRANE)
    , membrane(of)
  {}

  void setChems(const Point5d& c, const Point5d& dc)
  {
    switch(membrane.orientation()) {
      case cellflips::pos:
        membrane->PINpos = c[PIN];
        membrane->dPINpos = dc[PIN];
        membrane->APINpos = c[APIN];
        membrane->dAPINpos = dc[APIN];
        membrane->AAUXpos = c[AAUX];
        membrane->dAAUXpos = dc[AAUX];
        membrane->VAFpos = c[VAF];
        membrane->dVAFpos = dc[VAF];
        break;
      case cellflips::neg:
        membrane->PINneg = c[PIN];
        membrane->dPINneg = dc[PIN];
        membrane->APINneg = c[APIN];
        membrane->dAPINneg = dc[APIN];
        membrane->AAUXneg = c[AAUX];
        membrane->dAAUXneg = dc[AAUX];
        membrane->VAFneg = c[VAF];
        membrane->dVAFneg = dc[VAF];
        break;
      default:
        break;
    }
  }

  void update(Point5d& c, Point5d& dc)
  {
    switch(membrane.orientation()) {
      case cellflips::pos:
        c[PIN] = membrane->PINpos;
        c[APIN] = membrane->APINpos;
        c[AAUX] = membrane->AAUXpos;
        c[VAF] = membrane->VAFpos;
        dc[PIN] = membrane->dPINpos;
        dc[APIN] = membrane->dAPINpos;
        dc[AAUX] = membrane->dAAUXpos;
        dc[VAF] = membrane->dVAFpos;
        break;
      case cellflips::neg:
        c[PIN] = membrane->PINneg;
        c[APIN] = membrane->APINneg;
        c[AAUX] = membrane->AAUXneg;
        c[VAF] = membrane->VAFneg;
        dc[PIN] = membrane->dPINneg;
        dc[APIN] = membrane->dAPINneg;
        dc[AAUX] = membrane->dAAUXneg;
        dc[VAF] = membrane->dVAFneg;
        break;
      default:
        break;
    }
  }
};

struct ApoplastLink : public SolverLink
{
  face apoplast;

  ApoplastLink(const face& apo)
    : SolverLink(NT_APOPLAST)
    , apoplast(apo)
  {}

  void setChems(const Point5d& c, const Point5d& dc)
  {
    apoplast->auxin = c[AUXIN];
    apoplast->VAF = c[VAF];
    apoplast->dauxin = dc[AUXIN];
    apoplast->dVAF = dc[VAF];
  }

  void update(Point5d& c, Point5d& dc)
  {
    c[AUXIN] = apoplast->auxin;
    c[VAF] = apoplast->VAF;
    dc[AUXIN] = apoplast->dauxin;
    dc[VAF] = apoplast->dVAF;
  }
};

// Graph used to represent the set of ODEs
// This allows the use of the standard ODE solver
 

#line 245 "structure.vvh"

    
  struct p975758e3_f14b_11e7_aac5_3417eba08742_vertex_content {
    typedef p975758e3_f14b_11e7_aac5_3417eba08742_vertex_content Self;

#line 247 "structure.vvh"

    //std::unique_ptr<SolverLink> link;
    SolverLink *link;
    NodeType type;
    bool is_L1 = false;  // for cells and membranes in the L1
    bool is_sink_membrane = false;  // for membranes of sink cells
    Point5d c, dc;
    double size; // volume or area, depending on the dimension of the item
    RDSolver::VertexInternals interns;
    Point3d normal; // Normal to the membrane or the cell

    void apply() { link->setChems(c, dc); }
    void read() { link->update(c, dc); }

    //void setLink(std::unique_ptr<SolverLink>&& l)
    void setLink(SolverLink *l)
    {
      //link = std::move(l);
      link = l;
      type = l->type;
    }
  };

#line 268 "structure.vvh"


    
  struct p975758e3_f14b_11e7_aac5_3417eba08742_edge_content {
    typedef p975758e3_f14b_11e7_aac5_3417eba08742_edge_content Self;

#line 271 "structure.vvh"

    RDSolver::EdgeInternals interns;
    double area;  // used for the area between two neighbor apoplast elements
    double length;  // used for the interface length between two neighbor membrane elements
  };

#line 275 "structure.vvh"

typedef graph::VVGraph<p975758e3_f14b_11e7_aac5_3417eba08742_vertex_content, p975758e3_f14b_11e7_aac5_3417eba08742_edge_content, false> SolverGraph;
typedef SolverGraph::arc_t arc;
typedef SolverGraph::edge_t nlink;
typedef SolverGraph::const_edge_t const_nlink;
typedef SolverGraph::vertex_t node;

#line 276 "structure.vvh"


#endif // STRUCTURE_VVH
