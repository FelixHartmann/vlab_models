#ifndef CELLTUPLES_H
#define CELLTUPLES_H

#include <util/static_assert.h>

namespace ndcomplex
{

template <int _N, typename CellComplex>
struct TupleConstructor_
{
  STATIC_ASSERT(_N>=0, "Dimension must be greater than 0");
  typedef typename TupleConstructor_<_N-1,CellComplex>::type prev_type;
  typedef typename PrependTuple<typename NTypes<CellComplex::N-_N, CellComplex>::cell_t, prev_type>::type type;
};

template <typename CellComplex>
struct TupleConstructor_<0, CellComplex>
{
  typedef std::tuple<typename NTypes<CellComplex::N,CellComplex>::cell_t> type;
};

/**
 * \ingroup main
 * \class CellTuple celltuples.h "celltuples.h"
 *
 * Class implementing the notion of cell tuple for a given cell complex
 */
template <typename CellComplex>
struct CellTuple
{
  enum {
    N = CellComplex::N ///< Dimension of the tuple
  };
  typedef typename TupleConstructor_<N, CellComplex>::type tuple_type;
  tuple_type cell_tuple;

  typedef typename CellComplex::template ncell_t<0>::cell_t vertex_t;
  typedef typename CellComplex::template ncell_t<1>::cell_t edge_t;
  typedef typename CellComplex::template ncell_t<N-1>::cell_t face_t;
  typedef typename CellComplex::template ncell_t<N>::cell_t cell_t;

  /**
   * Creates an empty cell tuple
   */
  CellTuple()
    : cell_tuple()
  { }

  CellTuple(const CellTuple& ) = default;
  CellTuple(CellTuple&&) = default;

  CellTuple& operator=(const CellTuple&) = default;
  CellTuple& operator=(CellTuple&&) = default;

  /**
   * Create an initialize cell tuple, with each dimension initialized.
   *
   * The cells must be given in increasing dimension, starting at 0.
   */
  template <typename... Args>
  CellTuple(Args... args)
    : cell_tuple(args...)
  { }

  /**
   * Returns true if the tuple is valid (i.e. all cells are valid)
   */
  explicit operator bool() const
  {
    return isValid();
  }

  /**
   * Returns true if the tuple is valid (i.e. all cells are valid)
   */
  bool isValid() const
  {
    return isValid<N>();
  }

  /**
   * Returns the tuple flipped on dimension N1
   */
  template <int N1>
  CellTuple flipped(const CellComplex& C) const
  {
    CellTuple copy(*this);
    if(copy.flip<N1>(C))
      return copy;
    return CellTuple();
  }

  /**
   * Flip the dimension N1 and returns true if the tuple exists
   */
  template <int N1>
  bool flip(const CellComplex& C)
  {
    STATIC_ASSERT(N1 >= 0 and N1 <= N, "Dimension must be from 0 to N (inclusive)");
    typename CellComplex::template ncell_t<N1>::cell_t f = flip_<N1>(C, typename test_type<N1==0>::type(), typename test_type<N1==N>::type());
    if(f)
    {
      std::get<N1>(cell_tuple) = f;
      return true;
    }
    return false;
  }

  /**
   * Returns the cell of dimension N1 in the tuple
   */
  template <int N1>
  const typename CellComplex::template ncell_t<N1>::cell_t& ncell() const
  {
    STATIC_ASSERT(N1 >= 0 and N1 <= N, "Dimension must be from 0 to N (inclusive)");
    return std::get<N1>(cell_tuple);
  }

  /**
   * Returns the cell of dimension N1 in the tuple
   */
  template <int N1>
  typename CellComplex::template ncell_t<N1>::cell_t& ncell()
  {
    STATIC_ASSERT(N1 >= 0 and N1 <= N, "Dimension must be from 0 to N (inclusive)");
    return std::get<N1>(cell_tuple);
  }

  /**
   * Returns the 0-cell in the tuple
   */
  const vertex_t& vertex() const
  {
    return ncell<0>();
  }

  /**
   * Returns the 0-cell in the tuple
   */
  vertex_t& vertex()
  {
    return ncell<0>();
  }

  /**
   * Returns the 1-cell in the tuple
   */
  const edge_t& edge() const
  {
    return ncell<1>();
  }

  /**
   * Returns the 1-cell in the tuple
   */
  edge_t& edge()
  {
    return ncell<1>();
  }

  /**
   * Returns the N-1-cell in the tuple
   */
  const face_t& face() const
  {
    return ncell<N-1>();
  }

  /**
   * Returns the N-1-cell in the tuple
   */
  face_t& face()
  {
    return ncell<N-1>();
  }

  /**
   * Returns the N-cell in the tuple
   */
  const cell_t& cell() const
  {
    return ncell<N>();
  }

  /**
   * Returns the N-cell in the tuple
   */
  cell_t& cell()
  {
    return ncell<N>();
  }

  /**
   * Returns the orientation of the cell tuple.
   *
   * The orientation of the cell tuple is the product of the relative orientation between each successive pair of cells.
   */
  RelativeOrientation orientation(const CellComplex& C) const
  {
    return C.relativeOrientation(C.T, cell()) * orientation_<N>(C, false_type());
  }

  /**
   * Returns the orientation of the cell tuple.
   *
   * The orientation of the cell tuple is the product of the relative orientation between each successive pair of cells.
   */
  template <int N1, int N2>
  RelativeOrientation relativeOrientation(const CellComplex& C) const
  {
    STATIC_ASSERT(N1<=N2, "The first dimension must be the smallest");
    return relativeOrientation_<N1,N2>(C, typename test_type<(N1==N2)>::type());
  }

  protected:

  template <int N>
  bool isValid(const true_type&) const
  {
    return (bool)(ncell<N>()) and isValid<N-1>();
  }

  template <int N>
  bool isValid(const false_type&) const
  {
    return true;
  }

  template <int N>
  bool isValid() const
  {
    return isValid(typename test_type<(N>=0)>::type());
  }

  template <int N1>
  typename CellComplex::template ncell_t<N1>::cell_t flip_(const CellComplex& C, const false_type&, const false_type&)
  {
    return C.flip(ncell<N1+1>(), ncell<N1>(), ncell<N1-1>());
  }

  // case N1 == 0
  template <int N1>
  typename CellComplex::template ncell_t<N1>::cell_t flip_(const CellComplex& C, const true_type&, const false_type&)
  {
    return C.flip(ncell<N1+1>(), ncell<N1>(), C._);
  }

  // case N1 == N
  template <int N1>
  typename CellComplex::template ncell_t<N1>::cell_t flip_(const CellComplex& C, const false_type&, const true_type&)
  {
    return C.flip(C.T, ncell<N1>(), ncell<N1-1>());
  }

  template <int N1>
  RelativeOrientation orientation_(const CellComplex& C, const false_type&) const
  {
    return C.relativeOrientation(ncell<N1>(), ncell<N1-1>()) * orientation_<N1-1>(C, typename test_type<(N1==1)>::type());
  }

  // Case N1 == 0
  template <int N1>
  RelativeOrientation orientation_(const CellComplex& C, const true_type&) const
  {
    return C.relativeOrientation(ncell<N1>(), C._);
  }

  template <int N1, int N2>
  RelativeOrientation relativeOrientation_(const CellComplex& C, const false_type&) const
  {
    return C.relativeOrientation(ncell<N2>(), ncell<N2-1>())*relativeOrientation<N1,N2-1>(C);
  }

  template <int N1, int N2>
  RelativeOrientation relativeOrientation_(const CellComplex&, const true_type&) const
  {
    return pos;
  }
};

template <int N1, typename CellComplex>
QString cell_tuple_toString__(const CellTuple<CellComplex>& t, const false_type&)
{
  return QString("%1,%2").arg(shortString(t.template ncell<N1>())).arg(cell_tuple_toString__<N1-1>(t));
}

template <int N1, typename CellComplex>
QString cell_tuple_toString__(const CellTuple<CellComplex>& t, const true_type&)
{
  return shortString(t.template ncell<0>());
}

template <int N1, typename CellComplex>
QString cell_tuple_toString__(const CellTuple<CellComplex>& t)
{
  return cell_tuple_toString__<N1>(t, typename test_type<(N1==0)>::type());
}

template <typename CellComplex>
QString toString(const CellTuple<CellComplex>& t)
{
  return QString("tuple{%1}").arg(cell_tuple_toString__<CellComplex::N>(t));
}

template <typename CellComplex>
QTextStream& operator<<(QTextStream& s, const CellTuple<CellComplex>& t)
{
  s << toString(t);
  return s;
}

}

#endif // CELLTUPLES_H

