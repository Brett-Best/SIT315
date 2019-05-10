//
//  main.cpp
//  Quicksort - Sequential
//
//  Created by Brett Best on 23/4/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <vector>

using namespace std;

struct timespec startTimespec, endTimespec;

void startTimer() {
  clock_gettime(CLOCK_MONOTONIC, &startTimespec);
}

void stopTimer() {
  clock_gettime(CLOCK_MONOTONIC, &endTimespec);
}

vector<int> quicksort(vector<int> intVector) {
  if (intVector.size() > 1 == false) {
    return intVector;
  }
  
  int pivot = intVector[intVector.size()/2];
  
  vector<int> less, equal, greater;
  
  copy_if(intVector.begin(), intVector.end(), back_inserter(less), [&](int i) { return i < pivot; });
  copy_if(intVector.begin(), intVector.end(), back_inserter(equal), [&](int i) { return i == pivot; });
  copy_if(intVector.begin(), intVector.end(), back_inserter(greater), [&](int i) { return i > pivot; });
 
  vector<int> lessSorted = quicksort(less);
  vector<int> greaterSorted = quicksort(greater);
  
  vector<int> allSorted;
  
  copy(lessSorted.begin(), lessSorted.end(), back_inserter(allSorted));
  copy(equal.begin(), equal.end(), back_inserter(allSorted));
  copy(greaterSorted.begin(), greaterSorted.end(), back_inserter(allSorted));
  
  return allSorted;
}

double durationBetweenTimers() {
  double seconds = (endTimespec.tv_sec - startTimespec.tv_sec);
  seconds += (endTimespec.tv_nsec - startTimespec.tv_nsec) / 1000000000.0;
  
  return seconds;
}

vector<int> randomIntVector(int length) {
  vector<int> v(length);
  
  for (int index = 0; index < v.size(); index++) {
    v[index] = rand() % 1'000'000;
  }
  
  return v;
}

int main(int argc, const char * argv[]) {
  startTimer();
  
  vector<int> v = randomIntVector(10000);
  
  stopTimer();
  
  printf("Generate Vector: %.3fs\n", durationBetweenTimers());
  
  startTimer();
  
  for (int iteration = 1; iteration <= 10; iteration++) {
    vector<int> vSorted = quicksort(v);
  }
  
  stopTimer();
  
  printf("Sort Vectors: %.3fs\n", durationBetweenTimers());
  
  return 0;
}
