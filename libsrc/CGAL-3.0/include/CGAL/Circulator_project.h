// Copyright (c) 2003  Utrecht University (The Netherlands),
// ETH Zurich (Switzerland), Freie Universitaet Berlin (Germany),
// INRIA Sophia-Antipolis (France), Martin-Luther-University Halle-Wittenberg
// (Germany), Max-Planck-Institute Saarbruecken (Germany), RISC Linz (Austria),
// and Tel-Aviv University (Israel).  All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
// See the file LICENSE.LGPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $Source: /CVSROOT/CGAL/Packages/STL_Extension/include/CGAL/Circulator_project.h,v $
// $Revision: 1.53 $ $Date: 2003/10/21 12:23:34 $
// $Name: current_submission $
//
// Author(s)     : Michael Hoffmann <hoffmann@inf.ethz.ch>
//                 Lutz Kettner <kettner@mpi-sb.mpg.de>
//                 Sylvain Pion <Sylvain.Pion@mpi-sb.mpg.de>

#ifndef CGAL_CIRCULATOR_PROJECT_H
#define CGAL_CIRCULATOR_PROJECT_H 1
#include <CGAL/circulator.h>

CGAL_BEGIN_NAMESPACE

template < class C,
           class Fct,
           class Ref = typename C::reference,
           class Ptr = typename C::pointer>
class Circulator_project {
protected:
  C        nt;    // The internal circulator.
public:
  typedef C  Circulator;
  typedef Circulator_project<C,Fct,Ref,Ptr> Self;

  typedef typename  C::iterator_category   iterator_category;
  typedef typename  Fct::result_type       value_type;
  typedef typename  C::difference_type     difference_type;
  typedef typename  C::size_type           size_type;
  typedef           Ref                    reference;
  typedef           Ptr                    pointer;

  // CREATION
  // --------

  Circulator_project() {}
  Circulator_project( Circulator j) : nt(j) {}

  // OPERATIONS Forward Category
  // ---------------------------

  Circulator  current_circulator() const { return nt;}
  Ptr ptr() const {
    Fct fct;
    return &(fct(*nt));
  }

  bool operator==( CGAL_NULL_TYPE p) const {
    CGAL_assertion( p == 0);
    return ( nt == 0);
  }
  bool  operator!=( CGAL_NULL_TYPE p) const { return !(*this == p); }
  bool  operator==( const Self& i) const { return ( nt == i.nt); }
  bool  operator!=( const Self& i) const { return !(*this == i); }
  Ref   operator*()  const { return *ptr(); }
  Ptr   operator->() const { return ptr(); }
  Self& operator++() {
    ++nt;
    return *this;
  }
  Self  operator++(int) {
    Self tmp = *this;
    ++*this;
    return tmp;
  }

  // OPERATIONS Bidirectional Category
  // ---------------------------------

  Self& operator--() {
    --nt;
    return *this;
  }
  Self  operator--(int) {
    Self tmp = *this;
    --*this;
    return tmp;
  }

  // OPERATIONS Random Access Category
  // ---------------------------------

  Self  min_circulator() const {
    return Self( nt.min_circulator());
  }
  Self& operator+=( difference_type n) {
    nt += n;
    return *this;
  }
  Self  operator+( difference_type n) const {
    Self tmp = *this;
    return tmp += n;
  }
  Self& operator-=( difference_type n) {
    return operator+=( -n);
  }
  Self  operator-( difference_type n) const {
    Self tmp = *this;
    return tmp += -n;
  }
  difference_type  operator-( const Self& i) const {
    return nt - i.nt;
  }
  Ref  operator[]( difference_type n) const {
    Self tmp = *this;
    tmp += n;
    return tmp.operator*();
  }
};

template < class Dist, class Fct, class C, class Ref, class Ptr>
inline
Circulator_project<C,Fct,Ref,Ptr>
operator+( Dist n, Circulator_project<C,Fct,Ref,Ptr> i) { return i += n; }

CGAL_END_NAMESPACE
#endif // CGAL_CIRCULATOR_PROJECT_H //
// EOF //