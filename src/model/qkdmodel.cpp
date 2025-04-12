#include <iomanip>

#include <qkdmodel/qkdmodel.h>

void QKDModel::m_detect_gen_key_basis()
{
  for(int i = 0; i < m_init_key_length; i++)
  {
    //if bit is detected
    if(m_bernoulli_detect(m_rng))
    {
      m_detect_alice_key.push_back(m_int_uniform_01(m_rng));
      m_detect_alice_basis.push_back(m_int_uniform_01(m_rng));
    }
  }
}

//datw = detector and time windows
void QKDModel::m_get_datw()
{
  int time_window{0};
  //длина ключа, веткора базисов, векторов детекторов
  int key_size{m_detect_alice_key.size()};

  std::array<std::vector<int>, 2> detectors;
  //1ый детектор
  detectors[0] = std::vector<int>(key_size, 0);
  //2ой детектор
  detectors[1] = std::vector<int>(key_size, 0);

  for(int i = 0; i < key_size; i++)
  {
    //2 базис
    if (m_detect_alice_basis.at(i))
    {
      //получаем номер временного окна
      //0+0+1=1
      //0+1+1=2 
      //1+0+1=2
      //1+1+1=3
      //P(time_window=1)=1/4, P(time_window=2)=1/2, P(time_window=3)=1/4
      time_window = m_int_uniform_01(m_rng) + m_int_uniform_01(m_rng) + 1;
     
      //если попали во 2 временное окно, то устанавливаем значение соотвествующего детектора в зависимости от значения бита ключа
      if (time_window == 2)
        detectors[m_detect_alice_key.at(i)].at(i)=time_window;

      //попали в 1 или 3 временное окно
      //выбираем один из двух детекторов для конкретного временного окна (P=1/4*1/2=1/8) и записываем значение
      else
        detectors[m_int_uniform_01(m_rng)].at(i)=time_window;
    }
    //1 базис
    else
    {
      //если бит ключа равен 0, то time_window равен 1 или 2
      //если бит ключа равен 1, то time_window равен 2 или 3
      //поэтому получаем следующую формулу
      time_window =  m_int_uniform_01(m_rng) + m_detect_alice_key.at(i) + 1;

      //выбираем один из двух детекторов для конкретного временного окна и записываем значение
      detectors[m_int_uniform_01(m_rng)].at(i)=time_window;
    }
  }
  m_detector_1 = detectors[0];
  m_detector_2 = detectors[1];
}


void QKDModel::m_sift_bob()
{
  //длина ключа, веткора базисов, векторов детекторов
  int length = m_detect_alice_key.size();
  //считаем все биты неопределенными
  m_bob_confidence = std::vector<bool>(length, 0);

  for(int i = 0; i < length; i++)
  {
    //2 базис
    if (m_detect_alice_basis.at(i)) 
    {
      //1 детектор, 2 временное окно => бит равен 0
      if (m_detector_1.at(i) == 2)
      {
        m_bob_confidence.at(i) = 1;
        m_sift_bob_key.push_back(0);
      }
      //2 детектор, 2 временное окно => бит равен 1
      if (m_detector_2.at(i) == 2)
      {
        m_bob_confidence.at(i) = 1;
        m_sift_bob_key.push_back(1);
      }
    }
    //1 базис
    //2 временное окно => бит неопределен
    //(все биты считаются изначально неопределенными при определении вектора m_bob_confidence)
    //в противном случае
    else if (!(m_detector_1.at(i) == 2 || m_detector_2.at(i) == 2))
    {
      //1 временное окно => бит равен 0
      //3 временное окно => бит равен 1
      m_bob_confidence.at(i) = 1;
      m_sift_bob_key.push_back((m_detector_1.at(i) + m_detector_2.at(i) - 1) / 2);
    }
  }
}

void QKDModel::m_sift_alice()
{
  for(int i = 0; i < m_bob_confidence.size(); i++)
  {
    //кладем биты, в которых Боб уверен 
    if (m_bob_confidence.at(i))
      m_sift_alice_key.push_back(m_detect_alice_key[i]);
  }

  if (m_sift_alice_key.empty())
    throw "sift alice key is empty";
}

int QKDModel::m_inject_errors() {
  int flip_count{0};
  for(int i = 0; i < m_sift_bob_key.size(); i++)
  {
    if (m_bernoulli_qber(m_rng))
    {
      m_sift_bob_key.at(i) = !m_sift_bob_key.at(i);
      flip_count++;
    }
  }
  return flip_count;
}

void QKDModel::m_calculate_errors()
{
  int length{m_sift_alice_key.size()};

  for(int i = 0; i < length; i++)
  {
    if (m_sift_alice_key.at(i) != m_sift_bob_key.at(i))
    {
      m_error_count++;
    }
  }


  m_error_percentage = m_calculate_percentage(length, m_error_count);
  m_decode_success = (m_error_percentage <= DEFAULT_ERROR_THRESHOLD_PERCENTAGE);
}

void QKDModel::QKDiter()
{
      m_detect_gen_key_basis();
      m_get_datw();
      m_sift_bob();
      m_sift_alice();


      int flip_count{m_inject_errors()};
      double flip_percent{m_calculate_percentage(m_sift_bob_key.size(), flip_count)};
      //закомментировал и заменил на строку выше
      //double flip_percent = sift_bob_key.empty() ? 0.0f : (static_cast<double>(flip_count) / sift_bob_key.size()) * 100.0f;
      std::cout << "Injected errors: " << flip_count << " bits flipped (" << flip_percent << "%)" << std::endl;

      m_calculate_errors();

      m_cout_key_length();
      if(m_table)
      {
        m_cout_table();
      }
}

int QKDModel::m_get_error_count() const {
  return m_error_percentage;
}

double QKDModel::m_get_error_percentage() const {
  return m_error_percentage;
}

bool QKDModel::m_get_decode_success() const {
  return m_decode_success;
}

int QKDModel::m_get_detect_key_size() const {
  return m_detect_alice_key.size();
}

int QKDModel::m_get_sift_key_size() const {
  return m_sift_bob_key.size();
}

double QKDModel::m_calculate_percentage(int f, int l) const {
  return static_cast<double>(l) / f * 100; 
}

void QKDModel::m_cout_key_length() const
{
  std::cout << "phase\t\tkey_length\t%\t\terrors\t\terror%\t\tdecoding" << std::endl;

  std::cout << "INIT:\t\t";
  std::cout << m_init_key_length << "\t\tx\t\t-\t\t-\t\t-" << std::endl;

  std::cout << "DETECT:\t\t";
  std::cout << m_get_detect_key_size() << "\t\t" << m_calculate_percentage(m_init_key_length, m_get_detect_key_size()) << "\t\t-\t\t-\t\t-" << std::endl;

  std::cout << "SIFT:\t\t";
  std::cout << m_get_sift_key_size() << "\t\t" 
            << m_calculate_percentage(m_get_detect_key_size(), m_get_sift_key_size()) 
            << "\t\t" << m_get_error_count() << "\t\t" 
            << std::fixed << std::setprecision(2) << m_get_error_percentage() << "%\t\t"
            << (m_get_decode_success() ? "SUCCESS" : "FAIL") << std::endl;
  std::cout << std::endl;
}

void QKDModel::m_cout_table() const
{
  std::cout << std::setfill('-') 
            << std::setw(50) 
            << ' ' 
            << std::endl;

  std::cout << "|pos"   << "\t|"
            << "bit"    << "\t|"
            << "basis"  << "\t|"
            << "t_win"  << "\t|"
            << "d"      << "\t|"
            << "conf"   << "\t|"
            << std::endl;

  std::cout << std::setfill('-') 
            << std::setw(50) 
            << ' ' 
            << std::endl;

  for (int i = 0; i < m_get_detect_key_size(); i++)
  {
  std::cout << '|' << i                    << "\t|"
            << m_detect_alice_key.at(i)      << "\t|"
            << m_detect_alice_basis.at(i)+1  << "\t|"
            << m_detector_1.at(i)+m_detector_2.at(i)                 << "\t|"
            << ((m_detector_1.at(i)==0)?"2":"1")        << "\t|"
            << m_bob_confidence.at(i)                 << "\t|"
            << std::endl;
  }
}
