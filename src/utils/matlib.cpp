#include "matlib.h"

namespace ff
{

    double        rand(void){
      return 1.0 * ::rand() / RAND_MAX ;
    }

    FMatrix      rand(int m, int n)
    {
        FMatrix res(m, n);
        for(int i = 0; i < m; ++i)
          for(int j = 0; j < n; ++j)
          {
            res(i, j) = rand();
          }
        return res;
    }

    void randperm(int n, std::vector<int> & iVector)
    {
      iVector.clear();
      for(int i = 0; i < n; ++i)
	iVector.push_back(i);
      std::random_shuffle(iVector.begin(), iVector.end());
    }

    FMatrix  zeros(int m, int n)
    {
      FMatrix res(m, n, 0.0);
      return res;
    }

    FMatrix     ones(int m, int n)
    {
      FMatrix res(m, n, 1.0);
      return res;
    }
    
    FMatrix add(const FMatrix & m, const FMatrix & m1)
    {
	if(m.rows() == 0)
	  return m1;
	if(m1.rows() == 0)
	  return m;
	FMatrix res = m;
	res += m1;
        return res;
    }    
    
    FMatrix sub(const FMatrix & m, const FMatrix & m1)
    {
	FMatrix res = m;
	res -= m1;
        return res;
    }
    
    FMatrix     mul(const FMatrix & m, const FMatrix & m1 )
    {
	FMatrix res = m;
	res *= m1;
	return res;
    }
    
    ////////////////
    FMatrix trans(const FMatrix & m)
    {
        return blaze::trans(m);
    } 
    
    ///////////////////
    FMatrix bitWiseMul(const FMatrix & m, const FMatrix & m1)
    {
	if(m.rows() != m1.rows() || m.columns() != m1.columns())
	{
	   std::cout << "Warning: bit-wise multiplication must be in the same size!" << std::endl;
	   return m;
	}
        FMatrix res(m.rows(), m.columns());
        for(size_t i = 0; i < m.rows(); ++i)
        {
           for(size_t j = 0; j < m.columns(); ++j)
           {
             res(i, j) = m(i, j) * m1(i, j);
           }
        }
        return res;
    }
    
    FMatrix     delPreColumn(const FMatrix & m )//delete first column of m
    {
	FMatrix res(m.rows(),m.columns() -1);
	res = submatrix(m,0UL,1UL,m.rows(),m.columns()-1);
	return res;
    }
    FMatrix     columnMean(const FMatrix & m )//caculate mean for each column
    {
	FMatrix res(1, m.columns());
	for(size_t i = 0; i < m.columns(); ++i)
	{
	  double sum = 0.0;
	  for(size_t j = 0; j < m.rows(); ++j)
	  {
	    sum += m(j,i);	     
	  }
	  res(0,i) = sum / m.rows();
	}
	return res;
    }
    FColumn     rowMaxIndexes(const FMatrix & m )//find max for each row
    {
	FColumn res(m.rows(),0);
	for(size_t i = 0; i < m.rows(); ++i)
	{
	  double max = m(i,0);
	  for(size_t j = 1; j < m.columns(); ++j)
	  {
	    if(m(i,j) > max)
	    {
	      max = m(i,j);
	      res[i] = j;
	    }
	  }
	}
	return res;
    }    
    std::vector<int>	findUnequalIndexes(const FColumn & c, const FColumn & c1)//find unequal indexes in two columns
    {
      std::vector<int> res;
      if(c.size() != c1.size()){
	std::cout << "Warning: column sizes are different!\n" << std::endl;
	res.push_back(-1);//insert -1 as wrong outputs.
	return res;
      }
      for(int i = 0; i < c.size(); i++)
      {
	if(c[i] != c1[i])
	  res.push_back(i);
      }     
      return res;
    }
    double     matrixSum(const FMatrix & m )//caculate mean for each column
    {
	double sum = 0.0;
	for(size_t i = 0; i < m.rows(); ++i)
	{
	  for(size_t j = 0; j < m.columns(); ++j)
	  {
	    sum += m(i,j);	     
	  }
	}
	return sum;
    }
    FMatrix bitWiseSquare(const FMatrix & m)
    {
	FMatrix res(m.rows(), m.columns());
        for(size_t i = 0; i < m.rows(); ++i)
        {
           for(size_t j = 0; j < m.columns(); ++j)
           {
             res(i, j) = m(i, j) * m(i, j);
           }
        }
        return res;
    }
    FMatrix bitWiseLog(const FMatrix & m)
    {
	FMatrix res(m.rows(), m.columns());
        for(size_t i = 0; i < m.rows(); ++i)
        {
           for(size_t j = 0; j < m.columns(); ++j)
           {
             res(i, j) = log(m(i, j));
           }
        }
        return res;
    }
    FMatrix repmat(const FMatrix & m, int rowX, int columnX)
    {
	FMatrix res(m.rows() * rowX, m.columns() * columnX);
	for(size_t i = 0; i < rowX; ++i)
	{
	  for(size_t j = 0; j < columnX; ++j)
	  {
	    submatrix(res, i * m.rows(), j * m.columns(), m.rows(), m.columns()) = m;
	  }
	}
	return res;
    }
    
};//end namespace ff    

    ff::FMatrix operator + (const ff::FMatrix & t1, const ff::FMatrix & t )
    {
      return ff::add(t1, t);
    }
        
    ff::FMatrix operator - (const ff::FMatrix & t1, const ff::FMatrix & t )
    {
      return ff::sub(t1, t);
    }
    
    ff::FMatrix operator * (const ff::FMatrix & t1, const ff::FMatrix & t )
    {
      return ff::mul(t1, t);
    }
    