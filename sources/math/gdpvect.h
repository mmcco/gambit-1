//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Doubly-partitioned vector class
//

#ifndef GDPVECT_H
#define GDPVECT_H

#include "math/gpvector.h"

template <class T> class gDPVector : public gPVector<T>  {
  private:
    int sum(int part, const gPVector<int> &v) const;
    void setindex(void);

    bool Check(const gDPVector<T> &) const;

  protected:
    T ***dvptr;
    gArray<int> dvlen, dvidx;

  public:
    gDPVector(void);
    gDPVector(const gPVector<int> &sig);
    gDPVector(const gVector<T> &val, const gPVector<int> &sig);
    gDPVector(const gDPVector<T> &v);
    virtual ~gDPVector();

    T &operator()(int a, int b, int c);
    const T &operator()(int a, int b, int c) const;

	// extract a subvector
    void CopySubRow(int row, int col,  const gDPVector<T> &v);

    gDPVector<T> &operator=(const gDPVector<T> &v);
    gDPVector<T> &operator=(const gPVector<T> &v);
    gDPVector<T> &operator=(const gVector<T> &v);
    gDPVector<T> &operator=(T c);

    gDPVector<T> operator+(const gDPVector<T> &v) const;
    gDPVector<T> &operator+=(const gDPVector<T> &v);
    gDPVector<T> operator-(void) const;
    gDPVector<T> operator-(const gDPVector<T> &v) const;
    gDPVector<T> &operator-=(const gDPVector<T> &v);
    T operator*(const gDPVector<T> &v) const;
    gDPVector<T> &operator*=(const T &c);
    gDPVector<T> operator/(const T &c) const;
    
    bool operator==(const gDPVector<T> &v) const;
    bool operator!=(const gDPVector<T> &v) const;

    const gArray<int> &DPLengths(void) const;

    void Dump(gOutput &) const;
};

#ifndef __BORLANDC__
template <class T> gOutput &operator<<(gOutput &to, const gDPVector<T>&v);
#endif

#endif   // GDPVECTOR_H


