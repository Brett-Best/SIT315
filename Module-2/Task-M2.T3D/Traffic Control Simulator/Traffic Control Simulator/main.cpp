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

#define PRODUCERS_TO_USE 4
#define CONSUMERS_TO_USE 4
#define TRAFFIC_LIGHTS 5

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
bool producersCompleted = false;

mutex trafficSignalEntriesReadMutex;

struct TrafficSignalEntry {
  long long timestamp;
  int id;
  int numberOfCars;
};

#pragma mark -

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
        trafficSignalEntriesReadMutex.lock();
        
        if (trafficSignalEntriesRead == line) {
          processed = true;
          trafficSignalEntriesRead++;
          trafficSignalEntries.push_back(trafficSignalEntry);
        }
        
        trafficSignalEntriesReadMutex.unlock();
      }
      
      linesProcessed++;
      line++;
    }
  }
};

#pragma mark -

int numberOfCars[TRAFFIC_LIGHTS];
mutex numberOfCarsMutex;

int consumersFinished;
mutex consumersFinishedMutex;

class Consumer {
  
public:
  void processData(int threadId, int threadCount) {
    TrafficSignalEntry previousTrafficSignalEntry;
    long long maxTime = 0;
    
    while(!producersCompleted) {
      trafficSignalEntriesReadMutex.lock();
      
      if (trafficSignalEntries.size() == 0) {
        trafficSignalEntriesReadMutex.unlock();
        continue;
      }
      
      TrafficSignalEntry trafficSignalEntry = trafficSignalEntries[0];
      trafficSignalEntries.clear();
      
      trafficSignalEntriesReadMutex.unlock();
      
      if (maxTime == 0) {
        maxTime = trafficSignalEntry.timestamp + (60*60);
      }
      
      if (trafficSignalEntry.timestamp < maxTime) {
        numberOfCarsMutex.lock();
        numberOfCars[trafficSignalEntry.id] += trafficSignalEntry.numberOfCars;
        numberOfCarsMutex.unlock();
      } else {
        consumersFinishedMutex.lock();
        consumersFinished++;
        consumersFinishedMutex.unlock();
        printf("Consumer finished!\n");
        
        bool canContinue = false;
        while (!canContinue) {
          consumersFinishedMutex.lock();
          canContinue = consumersFinished == CONSUMERS_TO_USE;
          consumersFinishedMutex.unlock();
        }
        
        maxTime = 0;
      }
      
      previousTrafficSignalEntry = trafficSignalEntry;
    }
  }
  
  void sort(vector<TrafficSignalEntry> trafficSignalEntries, int threadId, int threadCount) {
    
  }
};

// (int [5]) ::numberOfCars = ([0] = 256, [1] = 176, [2] = 232, [3] = 321, [4] = 172)


#pragma mark -

int main(int argc, const char * argv[]) {
  
  thread producerThreads[PRODUCERS_TO_USE];
  thread consumerThreads[CONSUMERS_TO_USE];
  vector<Producer> producers;
  vector<Consumer> consumers;
  
  startTimer();
  
  for (int threadId = 0; threadId < PRODUCERS_TO_USE; threadId++) {
    Producer producer = Producer();
    
    producers.push_back(producer);
    producerThreads[threadId] = thread(&Producer::readCSV, producer, "trafficData.csv", threadId, PRODUCERS_TO_USE);
  }
  
  for (int threadId = 0; threadId < CONSUMERS_TO_USE; threadId++) {
    Consumer consumer = Consumer();
    
    consumers.push_back(consumer);
    consumerThreads[threadId] = thread(&Consumer::processData, consumer, threadId, CONSUMERS_TO_USE);
  }
  
  for (int threadId = 0; threadId < PRODUCERS_TO_USE; threadId++) {
    producerThreads[threadId].join();
  }
  
  producersCompleted = true;
  
  for (int threadId = 0; threadId < CONSUMERS_TO_USE; threadId++) {
    consumerThreads[threadId].join();
  }
  
  stopTimer();
  
  printf("Read CSV: %.3fs\n", durationBetweenTimers());
  
  return 0;
}
