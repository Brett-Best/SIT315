//
//  main.cpp
//  Matrix Multiplication - OpenMP
//
//  Created by Brett Best on 9/4/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

#include <iostream>
#include "omp.h"

using namespace std;

int main(int argc, const char * argv[]) {
#pragma omp parallel
  cout << "Greetings from thread " << omp_get_thread_num() << endl;
  return 0;
}
