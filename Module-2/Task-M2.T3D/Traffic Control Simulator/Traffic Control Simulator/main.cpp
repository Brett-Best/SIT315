//
//  main.cpp
//  Traffic Control Simulator
//
//  Created by Brett Best on 10/5/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>

using namespace std;

#define THREADS_TO_USE 3

#pragma mark - Timers
struct timespec startTimespec, endTimespec;

void startTimer() {
  clock_gettime(CLOCK_MONOTONIC, &startTimespec);
}

void stopTimer() {
  clock_gettime(CLOCK_MONOTONIC, &endTimespec);
}

double durationBetweenTimers() {
  double seconds = (endTimespec.tv_sec - startTimespec.tv_sec);
  seconds += (endTimespec.tv_nsec - startTimespec.tv_nsec) / 1000000000.0;
  
  return seconds;
}

#pragma mark - Traffic

struct TrafficSignalEntry;

vector<TrafficSignalEntry> trafficSignalEntries;
int trafficSignalEntriesRead = 0;

mutex trafficSignalEntriesMutex;

struct TrafficSignalEntry {
  long long timestamp;
  int id;
  int numberOfCars;
};

class Producer {
  
public:
  void readCSV(const string &csvPath, int threadId, int totalThreads) {
    ifstream csvFile;
    csvFile.open(csvPath.c_str());
    
    if (!csvFile.is_open()) {
      printf("Path is wrong!");
      exit(EXIT_FAILURE);
    }
    
    string lineData;
    getline(csvFile, lineData); // Skip header row
    
    int line = 0;
    int linesProcessed = 0;
    
    while (getline(csvFile,lineData)) {
      bool shouldProcess = (linesProcessed * totalThreads) + threadId == line;
      
      if (!shouldProcess) {
        line++;
        continue;
      }
      
      if (lineData.empty()) {
        continue; // Skip empty lines
      }
      
      istringstream iStringStream(lineData);
      string lineStream;
      string::size_type sz;
      
      vector <long long> row;
      
      while (getline(iStringStream, lineStream, ','))
      {
        row.push_back(stoll(lineStream,&sz));
      }
      
      TrafficSignalEntry trafficSignalEntry = TrafficSignalEntry { row[0], (int)row[1], (int)row[2] };
      
      bool processed = false;
      
      while (!processed) {
        trafficSignalEntriesMutex.lock();
        
        if (trafficSignalEntries.size() == line) {
          processed = true;
          trafficSignalEntries.push_back(trafficSignalEntry);
        }
        
        trafficSignalEntriesMutex.unlock();
      }
      
      
//      printf("Timestamp: %lli, Id: %i, Number of cars: %i -- Thread Id: %i, Line: %i\n", trafficSignalEntry.timestamp, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, threadId, line);
      
      linesProcessed++;
      line++;
    }
    
  }
};

#pragma mark -

int main(int argc, const char * argv[]) {
  
  thread threads[THREADS_TO_USE];
  vector<Producer> producers;
  
  startTimer();
  
  for (int runs = 0; runs < 10000; runs++) {
    
    for (int threadId = 0; threadId < THREADS_TO_USE; threadId++) {
      Producer producer = Producer();

      producers.push_back(producer);
      threads[threadId] = thread(&Producer::readCSV, producer, "trafficData.csv", threadId, THREADS_TO_USE);
    }
    
    for (int threadId = 0; threadId < THREADS_TO_USE; threadId++) {
      threads[threadId].join();
    }
    
    if (runs != 9999) {
    trafficSignalEntries.clear();
    }
  }
  
  stopTimer();
  
  printf("Read CSV: %.3fs\n", durationBetweenTimers());
  
  return 0;
}
