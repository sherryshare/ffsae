#include "sae/sae.h"
#include "dsource/read.h"
#include <time.h>
#include "nn/opt.h"

using namespace ff;

int main(int argc, char *argv[])
{
    srand(time(NULL));
    TData d = read_data();
    *d.train_x = (*d.train_x) / 255;
    *d.test_x = (*d.test_x) / 255;   
    
    //add for quick test
    *d.train_x = submatrix(*d.train_x,0UL,0UL,opts::batchsize * 3,d.train_x->columns());
    *d.train_y = submatrix(*d.train_y,0UL,0UL,opts::batchsize * 3,d.train_y->columns());
    *d.test_x = submatrix(*d.test_x,0UL,0UL,opts::batchsize * 3,d.test_x->columns());
    *d.test_y = submatrix(*d.test_y,0UL,0UL,opts::batchsize * 3,d.test_y->columns());
    
    //train a 100 hidden unit SDAE and use it to initialize a FFNN
    //Setup and train a stacked denoising autoencoder (SDAE)
    std::cout << "Pretrain an SAE" << std::endl;
    ff::Arch_t c(2UL);
    c[0] = 784;
    c[1] = 100;    
    std::cout << "c = " << c << "numel(c) = " << numel(c) << std::endl;
    ff::SAE sae(c);
    //check if sae structure is correct
    std::vector<FBNN_ptr> & m_oAEs = sae.get_m_oAEs();
    for(int i = 0; i < m_oAEs.size(); i++)
    {
	std::vector<FMatrix_ptr> & m_oWs = m_oAEs[i]->get_m_oWs();
	std::vector<FMatrix_ptr> & m_oVWs = m_oAEs[i]->get_m_oVWs();
	std::vector<FMatrix_ptr> & m_oPs = m_oAEs[i]->get_m_oPs();
	std::cout << "SAE[" << i << "]:" << std::endl;
	for(int j = 0; j < m_oWs.size(); j++){
	  std::cout << "W[" << j << "] = {" << m_oWs[j]->rows() << ", " << m_oWs[j]->columns() << "}" << std::endl;
	  std::cout << "vW[" << j << "] = {" << m_oVWs[j]->rows() << ", " << m_oVWs[j]->columns() << "}" << std::endl;
	  std::cout << "P[" << j << "] = {" << m_oPs[j]->rows() << ", " << m_oPs[j]->columns() << "}" << std::endl;
	}
    }
    sae.SAETrain(*d.train_x);
    
    //Use the SDAE to initialize a FFNN
    std::cout << "Train an FFNN" << std::endl;
    ff:Arch_t cn(3UL);
    cn[0] = 784;
    cn[1] = 100;
    cn[2] = 10;
    std::cout << "cn = " << cn << "numel(cn) = " << numel(cn) << std::endl;
    ff::FBNN nn(cn,"sigm",1);
    //check if nn structure is correct
    std::vector<FMatrix_ptr> & m_oWs = nn.get_m_oWs();
    std::vector<FMatrix_ptr> & m_oVWs = nn.get_m_oVWs();
    std::vector<FMatrix_ptr> & m_oPs = nn.get_m_oPs();
    for(int j = 0; j < m_oWs.size(); j++) {
        std::cout << "W[" << j << "] = {" << m_oWs[j]->rows() << ", " << m_oWs[j]->columns() << "}" << std::endl;
        std::cout << "vW[" << j << "] = {" << m_oVWs[j]->rows() << ", " << m_oVWs[j]->columns() << "}" << std::endl;
        std::cout << "P[" << j << "] = {" << m_oPs[j]->rows() << ", " << m_oPs[j]->columns() << "}" << std::endl;
    }
    *m_oWs[0] = *(m_oAEs[0]->get_m_oWs())[0];//nn.W{1} = sae.ae{1}.W{1};
    
//     std::cout << "d.train_x = (" << d.train_x->rows() << "," << d.train_x->columns() << ")" << std::endl;
    
    //Train the FFNN
    nn.train(*d.train_x,*d.train_y);
    double error = nn.nntest(*d.test_x,*d.test_y);
    std::cout << "test error = " << error << std::endl;
    if(error >= 0.16)
      std::cout << "Too big error!" << std::endl;

    return 0;
}
