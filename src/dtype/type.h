#ifndef FFSAE_DTYPE_TYPE_H_
#define FFSAE_DTYPE_TYPE_H_

#include <blaze/Math.h>
// #include <blaze/math/DenseRow.h>
// #include <blaze/math/DenseColumn.h>
// #include <blaze/math/DenseSubmatrix.h>
#include <memory>

namespace ff
{
  typedef blaze::DynamicMatrix<double, blaze::rowMajor> FMatrix;
  typedef std::shared_ptr<FMatrix> FMatrix_ptr;
  typedef blaze::DynamicVector<int, blaze::columnVector> FColumn;
  
  template <class T>
  struct is_matrix{const static bool value=false;};

  template <>
  struct is_matrix<FMatrix>{const static bool value = true;};
    template<class T1, class T2>
    struct is_one_matrix_one_arith{
        static const bool value = (std::is_arithmetic<T1>::value || std::is_arithmetic<T2>::value) && (is_matrix<T1>::value || is_matrix<T2>::value);
    };//end struct is_one_matrix_one_arith;

    template <class T1, class T2>
      struct matrix_type{
        typedef typename std::conditional<is_matrix<T1>::value, T1, T2>::type type;
      };//end struct matrix_type
}

#endif
