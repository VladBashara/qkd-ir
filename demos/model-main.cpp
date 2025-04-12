#include <iostream>

#include <cxxopts.hpp>
#include "qkdmodel/qkdmodel.h"

struct parameters
{
  int length;
  double detect_prob;
  int iter;
  bool table;
};

void cout_parameters(const parameters& iParams)
{
  std::cout << "INPUT PARAMETERS" << std::endl;
  std::cout << "key_length\t" << iParams.length       <<std::endl;
  std::cout << "detect_prob\t"<< iParams.detect_prob  <<std::endl;
  std::cout << "iterations\t" << iParams.iter         <<std::endl;
  std::cout << "print_table\t"      << iParams.table        <<std::endl;
  std::cout << std::endl;
}

int main (int argc, char *argv[]) {
  cxxopts::Options options("Krk_model","");
  options.add_options()
    ("l,length","key length", cxxopts::value<int>()->default_value(std::to_string(DEFAULT_KEY_LENGTH)))
    ("p,dprob","Detect probability", cxxopts::value<double>()->default_value(std::to_string(DEFAULT_DETECTION_PROB)))
    ("i,iter","Number of iterations", cxxopts::value<int>()->default_value("1"))
    ("t,table", "Print table", cxxopts::value<bool>()->default_value("1"))
    ;

  auto result = options.parse(argc,argv);
  parameters input{
    result["length"].as<int>(),  
    result["dprob"].as<double>(),
    result["iter"].as<int>(),
    result["table"].as<bool>()
  };

  cout_parameters(input);

  int iter{input.iter}, n{input.iter+1};
  while (iter > 0)
  {
    std::cout << "Iteration " << n-iter << std::endl;
    QKDModel* qkd_model = new QKDModel(input.length,input.detect_prob, input.table);
    qkd_model->QKDiter(); 
    delete qkd_model; 
    iter--;
  }
  
  return 0;
}

