//src: http://stackoverflow.com/questions/10607215/simplest-tbb-example
//Ce code se contente d'afficher les valeur d'un vecteurs
//Evidement l'ordre d'apparition sera différents à chaque execution
//car on ne sait pas quel thread aura la main à quel moment
#include "tbb/parallel_for_each.h"
#include "tbb/task_scheduler_init.h"
#include <iostream>
#include <vector>

struct mytask {
  mytask(size_t n)
    :_n(n)
  {}
  void operator()() {
    for (int i=0;i<1000000;++i) {}  // Deliberately run slow
    std::cerr << "[" << _n << "]";
  }
  size_t _n;
};

template <typename T> struct invoker {
  void operator()(T& it) const {it();}
};

int main(int,char**) {

  tbb::task_scheduler_init init;  // Automatic number of threads
  // tbb::task_scheduler_init init(4);  // Explicit number of threads

  std::vector<mytask> tasks;
  for (int i=0;i<1000;++i)
    tasks.push_back(mytask(i));

  tbb::parallel_for_each(tasks.begin(),tasks.end(),invoker<mytask>());
  std::cerr << std::endl;

  return 0;
}
