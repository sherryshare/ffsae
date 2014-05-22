#include "nn/fbnn.h"


namespace ff
{
  FBNN::FBNN(const Arch_t & arch, std::string activeStr, int learningRate, double zeroMaskedFraction, bool testing, std::string outputStr)
      : m_oArch(arch)
      , m_iN(numel(arch))
      , m_strActivationFunction(activeStr)
      , m_iLearningRate(learningRate)
      , m_fInputZeroMaskedFraction(zeroMaskedFraction)
      , m_fTesting(testing)
      , m_strOutput(outputStr)
  {
      for(int i = 1; i < m_iN; ++i)
      {
	  FMatrix f = (rand(m_oArch[i], m_oArch[i-1] + 1) - 0.5) * (2 * 4 * sqrt(6/(m_oArch[i] + m_oArch[i-1])));//based on nnsetup.m
	  m_oWs.push_back(std::make_shared<FMatrix>(f));
	  FMatrix z = zeros(f.rows(), f.columns());
	  m_oVWs.push_back(std::make_shared<FMatrix>(z));
	  FMatrix p = zeros(1, m_oArch[i]);
	  m_oPs.push_back(std::make_shared<FMatrix>(p));
      }

  }
  
  //trains a neural net
  void FBNN::train(const FMatrix& train_x, const FMatrix& train_y, const FMatrix& valid_x, const FMatrix& valid_y)
  {
      int ibatchNum = train_x.rows() / opts.batchsize + (train_x.rows() % opts.batchsize != 0);
      FMatrix L = zeros(opts.numpochs * ibatchNum, 1);
      m_oLp = std::make_shared<FMatrix>(L);
      std::chrono::time_point<std::chrono::system_clock> start, end;
      int elapsedTime;
      Loss loss;
      for(int i = 0; i < opts.numpochs; ++i)
      {
	  start = std::chrono::system_clock::now();
	  std::vector<int> iRandVec;
	  randperm(train_x.rows(),iRandVec);
	  for(int j = 0; j < ibatchNum; ++j)
	  {
	      int curBatchSize = opts.batchsize;
	      if(j = ibatchNum - 1 && train_x.rows() % opts.batchsize != 0)
		  curBatchSize = train_x.rows() % opts.batchsize;
	      FMatrix batch_x(curBatchSize,train_x.columns());
	      for(int r = 0; r < curBatchSize; ++r)
		  row(batch_x,r) = row(train_x,iRandVec[j * opts.batchsize + r]);

	      //Add noise to input (for use in denoising autoencoder)
	      if(m_fInputZeroMaskedFraction != 0)
		  batch_x = bitWiseMul(batch_x,(rand(curBatchSize,train_x.columns())>m_fInputZeroMaskedFraction));

	      FMatrix batch_y(curBatchSize,train_y.columns());
	      for(int r = 0; r < curBatchSize; ++r)
		  row(batch_y,r) = row(train_y,iRandVec[j * opts.batchsize + r]);
	      
	      L(i*ibatchNum+j,0) = nnff(batch_x,batch_y);
	      nnbp();
	      nnapplygrads();
	  }
	  end = std::chrono::system_clock::now();
	  elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(end-start).count();
// 	  std::cout << "elapsed time: " << elapsedTime << "us" << std::endl;
	  //loss calculate use nneval
	  if(valid_x.rows() == 0 || valid_y.rows() == 0){
	    nneval(loss, train_x, train_y);
	    std::cout << "Full-batch train mse = " << loss.train_error.back() << " , val mse = " << loss.valid_error.back() << std::endl;
	  }
	  else{
	    nneval(loss, train_x, train_y, valid_x, valid_y);
	    std::cout << "Full-batch train mse = " << loss.train_error.back() << std::endl;
	  }
	  std::cout << "epoch " << i << " / " <<  opts.numpochs << " took " << elapsedTime << " seconds." << std::endl;
	  std::cout << "Mini-batch mean squared error on training set is " << columnMean(submatrix(L,0UL,i*ibatchNum,1UL,ibatchNum)) << std::endl;      
	  m_iLearningRate *= m_fScalingLearningRate;    
      }

  }
  void FBNN::train(const FMatrix & train_x, const FMatrix & train_y)
  {
      FMatrix emptyM;
      train(train_x,train_y,emptyM,emptyM);      
  }
  
  //NNFF performs a feedforward pass
  double FBNN::nnff(const FMatrix& x, const FMatrix& y)
  {
    double L = 0;
    if(m_oAs.empty())
    {
      for(int i = 0; i < m_iN; ++i)
	m_oAs.push_back(std::make_shared<FMatrix>(FMatrix()));
    }
    *m_oAs[0] = addPreColumn(x,1);
    
    if(m_fDropoutFraction > 0 && !m_fTesting)
    {
        if(m_odOMs.empty())//clear dropOutMask
        {
            for(int i = 0; i < m_iN - 1; ++i)
                m_odOMs.push_back(std::make_shared<FMatrix>(FMatrix()));
        }
    }
    //feedforward pass
    for(int i = 1; i < m_iN - 1; ++i)
    {
      //activation function
      if(m_strActivationFunction == "sigm")
      {	
	//Calculate the unit's outputs (including the bias term)
	*m_oAs[i] = sigm((*m_oAs[i-1]) * trans(*m_oWs[i-1]));
      }
      else if(m_strActivationFunction == "tanh_opt")
      {
	*m_oAs[i] = tanh_opt((*m_oAs[i-1]) * trans(*m_oWs[i-1]));
      }
      
      //dropout
      if(m_fDropoutFraction > 0)
      {
	if(m_fTesting)
	  *m_oAs[i] = (*m_oAs[i]) * (1 - m_fDropoutFraction);
	else
	{
	  *m_odOMs[i] = rand(m_oAs[i]->rows(),m_oAs[i]->columns()) > m_fDropoutFraction;
	  *m_oAs[i] = bitWiseMul(*m_oAs[i],*m_odOMs[i]);
	}
      }
      
      //calculate running exponential activations for use with sparsity
      if(m_fNonSparsityPenalty > 0)
	*m_oPs[i] =  (*m_oPs[i]) * 0.99 + columnMean(*m_oAs[i]);
	
      //Add the bias term
      *m_oAs[i] = addPreColumn(*m_oAs[i],1);    
    }
    
    if(m_strOutput == "sigm")
    {
      *m_oAs[m_iN -1] = sigm((*m_oAs[m_iN-2]) * trans(*m_oWs[m_iN-2]));
    }
    else if(m_strOutput == "linear")
    {
      *m_oAs[m_iN -1] = (*m_oAs[m_iN-2]) * trans(*m_oWs[m_iN-2]);
    }
    else if(m_strOutput == "softmax")
    {
      *m_oAs[m_iN -1] = softmax((*m_oAs[m_iN-2]) * trans(*m_oWs[m_iN-2]));
    }
    
    //error and loss
    m_oEp = std::make_shared<FMatrix>(y - (*m_oAs[m_iN-1]));
    
    if(m_strOutput == "sigm" || m_strOutput == "linear")
    {
      L = 0.5 * matrixSum(bitWiseSquare(*m_oEp)) / x.rows();
    }
    else if(m_strOutput == "softmax")
    {
      L = -matrixSum(bitWiseMul(y,bitWiseLog(*m_oAs[m_iN-1]))) / x.rows();
    }    
    return L;
  }
  
  //NNBP performs backpropagation
  void FBNN::nnbp(void)
  {
    FMatrix sparsityError;
    std::vector<FMatrix_ptr> oDs;
    //initialize oDs
    for(int i = 0; i < m_iN; i++)
      oDs.push_back(std::make_shared<FMatrix>(FMatrix()));
    if(m_strOutput == "sigm")
    {
      *oDs[m_iN -1] = bitWiseMul(*m_oEp,bitWiseMul(*m_oAs[m_iN -1],*m_oAs[m_iN -1] - 1));
    }
    else if(m_strOutput == "softmax" || m_strOutput == "linear")
    {
      *oDs[m_iN -1] = - (*m_oEp);
    }
    for(int i = m_iN - 2; i > 0; i--)
    {
      FMatrix d_act;
      if(m_strActivationFunction == "sigm")
      {	
	d_act = bitWiseMul(*m_oAs[i],1 - (*m_oAs[i]));
      }
      else if(m_strActivationFunction == "tanh_opt")
      {
	d_act = 1.7159 * 2/3 - 2/(3 * 1.7159) * bitWiseSquare(*m_oAs[i]);
      }
      
      if(m_fNonSparsityPenalty > 0)
      {
	FMatrix pi = repmat(*m_oPs[i],m_oAs[i]->rows(),1);
	sparsityError = addPreColumn(m_fNonSparsityPenalty * (1 - m_fSparsityTarget) / (1 - pi) - m_fNonSparsityPenalty * m_fSparsityTarget / pi,0);
      }
      
      //Backpropagate first derivatives
      if(i == m_iN - 2)//in this case in oDs there is not the bias term to be removed  
      {
	*oDs[i] = bitWiseMul(*oDs[i+1] * (*m_oWs[i]) + sparsityError,d_act);//Bishop (5.56)
      }
      else//in this case in oDs the bias term has to be removed
      {
	*oDs[i] = bitWiseMul(delPreColumn(*oDs[i+1]) * (*m_oWs[i]) + sparsityError,d_act);
      }
      
      if(m_fDropoutFraction > 0)
      {
	*oDs[i] = bitWiseMul(*oDs[i],addPreColumn(*m_odOMs[i],1));
      }
      
    }
    if(m_odWs.empty())//Initialize m_odWs
    {
        for(int i = 0; i < m_iN - 1; i++)
        {
            m_odWs.push_back(std::make_shared<FMatrix>(FMatrix()));
        }
    }    
    for(int i = 0; i < m_iN - 1; i++)
    {
      if(i == m_iN - 2)
      {
	*m_odWs[i] = trans(*oDs[i+1]) * (*m_oAs[i]) / oDs[i+1]->rows();
      }
      else
      {
	*m_odWs[i] = trans(delPreColumn(*oDs[i+1])) * (*m_oAs[i]) / oDs[i+1]->rows();
      }      
    }  

  }
  //updates weights and biases with calculated gradients
  void FBNN::nnapplygrads(void )
  {
    FMatrix dW;
    for(int i = 0; i < m_iN - 1; i++)
    {
      if(m_fWeithtPenaltyL2 > 0)
	dW = *m_odWs[i] + m_fWeithtPenaltyL2 * addPreColumn(delPreColumn(*m_oWs[i]),0);
      else
	dW = *m_odWs[i];
      dW = m_iLearningRate * dW;
      
      if(m_fMomentum > 0)
      {
	*m_oVWs[i] = (*m_oVWs[i]) * m_fMomentum + dW;
	dW = *m_oVWs[i];
      }
      
      *m_oWs[i] -= dW;       
    }
  }
  
  //evaluates performance of neural network
  void ff::FBNN::nneval(Loss & loss, const FMatrix& train_x, const FMatrix& train_y, const FMatrix& valid_x, const FMatrix& valid_y)
  {
    //training performance
    loss.train_error.push_back(nnff(train_x,train_y));
    
    //validation performance
    if(valid_x.rows() != 0 && valid_y.rows() != 0)
      loss.valid_error.push_back(nnff(valid_x,valid_y));
    
    //calc misclassification rate if softmax
    if(m_strOutput == "softmax")
    {
      loss.train_error_fraction.push_back(nntest(train_x,train_y));
      if(valid_x.rows() != 0 && valid_y.rows() != 0)
	loss.valid_error_fraction.push_back(nntest(valid_x,valid_y));
    }
  }

  void ff::FBNN::nneval(Loss & loss, const FMatrix& train_x, const FMatrix& train_y)
  {
    FMatrix emptyM;
    nneval(loss,train_x,train_y,emptyM,emptyM);
  }
  
  double ff::FBNN::nntest(const FMatrix& x, const FMatrix& y)
  {
    FColumn labels;
    nnpredict(x,y,labels);
    FColumn expected = rowMaxIndexes(y);
    std::vector<int> bad = findUnequalIndexes(labels,expected);
    return bad.size() / x.rows();//Haven't return bad vector.(nntest.m does)
  }
  
  void ff::FBNN::nnpredict(const FMatrix& x, const FMatrix& y, FColumn& labels)
  {
    m_fTesting = true;
    nnff(x,zeros(x.rows(),m_oArch[m_iN - 1]));
    m_fTesting = false;
    labels = rowMaxIndexes(*m_oAs[m_iN - 1]);
  }

}
