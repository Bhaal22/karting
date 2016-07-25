#include <iostream>
#include <ostream>
#include <vector>
#include <map>
#include <future>
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <limits>
#include <mutex>


using pass_time = std::chrono::system_clock::time_point;
using CheckpointInformation = std::pair<unsigned int, pass_time>;


class TimingSystem
{
public:
  static TimingSystem& instance()
  {
    static TimingSystem timing_system;
    return timing_system;
  }

  TimingSystem()
    : _mutex(), checkpoints(), start_time()
  { }
  
  void init()
  {
    auto start_time = std::chrono::steady_clock::now();
  }

  const std::vector<CheckpointInformation> &global_checkpoints()
  {
    return checkpoints;
  }

  const std::map<unsigned int, std::vector<std::chrono::system_clock::time_point> > &checkpoints_by_kart()
  {
    return passed_times_by_kart;
  }
  
  void record_kart(unsigned int kart_number, std::chrono::system_clock::time_point passing_time) {

    _mutex.lock();
    checkpoints.push_back(CheckpointInformation(kart_number, passing_time));

    std::vector<std::chrono::system_clock::time_point> laps = passed_times_by_kart[kart_number];

    passed_times_by_kart[kart_number].push_back(passing_time);
    _mutex.unlock();
  }

private:
  std::mutex _mutex;
  std::vector<CheckpointInformation> checkpoints;
  std::map<unsigned int, std::vector<pass_time> > passed_times_by_kart;
  std::chrono::system_clock::time_point start_time;
};

struct TeleportingStrategy
{
  static unsigned int random_factor()
  {
    static std::default_random_engine generator;
    static std::uniform_int_distribution<int> distribution(1, 30);

    return distribution(generator);
  }
  
  void drive(unsigned int start_position, unsigned int current_lap) const
  { 
    int factor = TeleportingStrategy::random_factor();
    if (current_lap == 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(600 * start_position));
    else
      std::this_thread::sleep_for(std::chrono::seconds(10 + factor));
  }
};


class Kart
{
public:
  Kart()
    : _number(0), _start_position(0), _laps_achieved(0)
  { }
  
  Kart(unsigned int number)
    : Kart()
  {
    _number = number;
  }

  Kart(const Kart &kart)
  {
    this->_number = kart._number;
    this->_start_position = kart._start_position;
  }

  void set_position(unsigned int position)
  {
    _start_position = position;
  }

  template <class DrivingStrategy>
  std::thread start(const DrivingStrategy &strategy, unsigned int maximum_laps)
  {
    
    return std::thread([this, &strategy, maximum_laps]()
                      {
                        while(this->_laps_achieved <= maximum_laps) {
                          strategy.drive(this->_start_position, this->_laps_achieved); 
                         
                          auto passing_time = std::chrono::system_clock::now();
                          auto now_c = std::chrono::system_clock::to_time_t(passing_time);
                          
                          char mbstr[100];
                          std::strftime(mbstr, sizeof(mbstr), "%T", std::localtime(&now_c));
                          
                          printf("kart %d Lap #%d - Passing Time %s\n", this->_number, this->_laps_achieved, mbstr);

                          this->checkpoint(passing_time);
                        }
                      });
  }

  void checkpoint(const std::chrono::system_clock::time_point &passing_time) {
    this->_laps_achieved++;
    auto &timing_system = TimingSystem::instance();
    timing_system.record_kart(_number, passing_time);
  }

  friend std::ostream& operator<<(std::ostream &oss, const Kart &kart)
  {
    oss << kart._number << ": " << kart._start_position;
    return oss;
  }
  
private:
  
  unsigned int _number;
  unsigned int _start_position;
  unsigned int _laps_achieved;
};


class Race
{
public:
  Race(std::vector<Kart> &karts, unsigned int laps)
    : _karts(karts), _laps(laps)
  {}

  void start()
  {
    std::cout << "Race has started" << std::endl;
    TimingSystem::instance().init();
    
    //std::vector<std::future<void> > kart_futures;
    std::vector<std::thread > kart_futures;
    for (auto &kart: _karts)
    {
      kart_futures.push_back(kart.start(TeleportingStrategy(), _laps));
    }

    for(auto &future : kart_futures)
    {
       future.join();
    }
  }

  void prepare()
  {
    std::vector<unsigned int> karts_ordering;
    for(int kart_number = 1; kart_number <= _karts.size(); kart_number++)
    {
      karts_ordering.push_back(kart_number);
    }
    
    std::random_device rd;
    std::mt19937 g(rd());
    
    std::shuffle(karts_ordering.begin(), karts_ordering.end(), g);

    std::cout << "Ignition Line Ordering" << std::endl;
    for(auto& kart: karts_ordering)
      std::cout << kart << " ";
    
    std::cout << std::endl;
    
    for (int pos = 0; pos < karts_ordering.size(); pos++)
    {
      auto kart_number = karts_ordering[pos];
      auto &kart = _karts[kart_number - 1];
      kart.set_position(pos);
    }
  }

  void finish()
  {
    std::cout << "Race has finished" << std::endl;
  }
  
private:
  std::vector<Kart> &_karts;
  unsigned int _laps;
};


void usage()
{
  std::cout << "Usage: ./kart_race #number_of_karts #laps" << std::endl;
}

void display_winner()
{
  auto checkpoints_by_karts = TimingSystem::instance().checkpoints_by_kart();

  //CheckpointInformation fastest_lap;

  unsigned int fastest_kart = -1;
  double fastest_lap = std::numeric_limits<unsigned int>::max();
  unsigned int nth_lap = 0;
  unsigned int fastest_nth_lap = 0;
  
  for(std::map<unsigned int, std::vector<pass_time> >::iterator it = checkpoints_by_karts.begin(); it != checkpoints_by_karts.end(); ++it) {
    const auto &passing_times = it->second;
    nth_lap = 0;
    
    if (passing_times.size() > 1)
    {
      auto passing_time_iterator = passing_times.begin();
      auto next = std::next(passing_time_iterator);
      while(next != passing_times.end())
      {
        nth_lap++;
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(next->time_since_epoch() - passing_time_iterator->time_since_epoch());
        std::cout << "Kart " << it->first << " " << dt.count() / 1000 << std::endl;

        if (dt.count() < fastest_lap) {
          fastest_lap = dt.count();
          fastest_kart = it->first;
          fastest_nth_lap = nth_lap;
        }

        passing_time_iterator = next;
        next = std::next(passing_time_iterator);
      }
    }
  }

  std::cout << "Winner is: " << "Kart " << fastest_kart << "(" << fastest_lap / 1000 << " sec) on lap " << fastest_nth_lap << std::endl;
}


int main(int argc, char **argv)
{
  if (argc < 3) {
    usage();

    return 1;
  }

  unsigned int number_of_karts = std::stoul(argv[1]);
  unsigned int laps = std::stoul(argv[2]);

  std::vector<Kart> karts;
  for(int kart_number = 1; kart_number <= number_of_karts; kart_number++)
  {
    karts.push_back(Kart(kart_number));
  }

  Race race(karts, laps);
  race.prepare();

  race.start();

  race.finish();

  display_winner();
  
  return 0;
}
