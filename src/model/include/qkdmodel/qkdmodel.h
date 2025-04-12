#ifndef QKD_MODEL_SOURCE_H 
#define QKD_MODEL_SOURCE_H

#include <iostream>
#include <random>
#include <array>
#include <vector>

#include "decoders.h"

// Константы
constexpr unsigned int  DEFAULT_KEY_LENGTH                 {1024};
constexpr double        DEFAULT_DETECTION_PROB             {0.5};
constexpr double        DEFAULT_ERROR_THRESHOLD_PERCENTAGE {50.0};
constexpr double        DEFAULT_QBER                       {0.05};

class QKDModel
{

  // Входные параметры
  int     m_init_key_length{DEFAULT_KEY_LENGTH};
  double  m_detect_prob{DEFAULT_DETECTION_PROB};
  double  m_qber{DEFAULT_QBER};
  bool    m_table{true};

  std::random_device m_rd   = std::random_device();
  std::mt19937       m_rng  = std::mt19937(m_rd());

  std::bernoulli_distribution m_bernoulli_detect = std::bernoulli_distribution(m_detect_prob);
  std::bernoulli_distribution m_bernoulli_qber   = std::bernoulli_distribution(m_qber);
  std::uniform_int_distribution<int> m_int_uniform_01 = std::uniform_int_distribution<int>(0,1);

  std::vector<bool> m_detect_alice_key{};
  std::vector<bool> m_detect_alice_basis{};

  std::vector<int> m_detector_1{};
  std::vector<int> m_detector_2{};

  std::vector<bool> m_bob_confidence{};
  std::vector<bool> m_sift_bob_key{};
  std::vector<bool> m_sift_alice_key{};
 

  int     m_error_count{0};
  double  m_error_percentage{0.0};
  bool    m_decode_success{false};
//  ...

  private:
    void m_detect_gen_key_basis();
    void m_get_datw();
    void m_sift_alice();
    void m_sift_bob();
    void m_calculate_errors();
    int m_inject_errors();

    int    m_get_error_count() const;
    double m_get_error_percentage() const;
    bool   m_get_decode_success() const;
    int    m_get_start_key_size() const;
    int    m_get_detect_key_size() const;
    int    m_get_sift_key_size() const;
    double m_calculate_percentage(int f, int l) const;

    void m_cout_key_length() const;
    void m_cout_table() const;

  public:
    QKDModel(){}
    ~QKDModel(){}
    QKDModel(int length, double loss_prob, bool f)
      : m_init_key_length(length), m_detect_prob(loss_prob), m_table(f) {}
    void QKDiter();
//    ...
};
#endif
