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

#define PRODUCERS_TO_USE 2
#define CONSUMERS_TO_USE 2
#define TRAFFIC_LIGHTS 3

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

int producersCompleted = 0;
mutex producersCompletedMutex;

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
          printf("PROCESSED - Thread Id: %i, Id: %i, Number of cars: %i\n", threadId, trafficSignalEntry.id, trafficSignalEntry.numberOfCars);
        }
        
        trafficSignalEntriesReadMutex.unlock();
      }
      
      linesProcessed++;
      line++;
    }
    
    producersCompletedMutex.lock();
    producersCompleted++;
    producersCompletedMutex.unlock();
  }
};

void printNumberOfCars();

#pragma mark -

int numberOfCars[TRAFFIC_LIGHTS];
mutex numberOfCarsMutex;

bool consumerWaiting[CONSUMERS_TO_USE];
mutex consumerWaitingMutex;

class Consumer {
  
  long long maxTime = 0;
  
public:
  void processData(int threadId, int threadCount) {
    bool shouldFinish = false;
    
    while(!shouldFinish) {
      shouldFinish = loadData(threadId, threadCount);
    }
  }
  
  bool loadData(int threadId, int threadCount) {
    trafficSignalEntriesReadMutex.lock();
    
    if (trafficSignalEntries.size() == 0) {
      handleEmptySignalEntries(threadId, threadCount);
      trafficSignalEntriesReadMutex.unlock();
      
      producersCompletedMutex.lock();
      if (producersCompleted == PRODUCERS_TO_USE) {
        producersCompletedMutex.unlock();
        return true;
      }
      producersCompletedMutex.unlock();
      
      return false;
    }

    TrafficSignalEntry trafficSignalEntry = trafficSignalEntries[0];
    trafficSignalEntries.erase(trafficSignalEntries.begin());
    
    trafficSignalEntriesReadMutex.unlock();
    
    printf("CONSUMED - Thread Id: %i, Id: %i, Number of cars: %i\n", threadId, trafficSignalEntry.id, trafficSignalEntry.numberOfCars);
    
    return false;
    
//    if (maxTime == 0) {
//      maxTime = trafficSignalEntry.timestamp + (60*60);
//      printf("Thread ID: %i, Max time: %lli\n", threadId, maxTime);
//    }
    
//    if (trafficSignalEntry.timestamp < maxTime) {
//      numberOfCarsMutex.lock();
//      numberOfCars[trafficSignalEntry.id] += trafficSignalEntry.numberOfCars;
//      if (0 == trafficSignalEntry.id) printf("CONSUMED - Thread Id: %i, Id: %i, Number of cars: %i, Total: %i\n", threadId, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, numberOfCars[trafficSignalEntry.id]);
//      numberOfCarsMutex.unlock();
//    } else {
//      consumerWaitingMutex.lock();
//      consumerWaiting[threadId] = true;
//      consumerWaitingMutex.unlock();
//
//      bool waitingForOtherThreads = true;
//
//      while (waitingForOtherThreads) {
//        consumerWaitingMutex.lock();
//        if (!consumerWaiting[threadId]) {
//          waitingForOtherThreads = false;
//          consumerWaitingMutex.unlock();
//          continue;
//        }
//
//        bool shouldContinue = true;
//
//        for (int i = 0; i < threadCount; i++) {
//          if (consumerWaiting[i] == false) shouldContinue = false;
//        }
//        consumerWaitingMutex.unlock();
//
//        if (shouldContinue) {
//          printNumberOfCars();
//          maxTime = 0;
//
//          numberOfCarsMutex.lock();
//          printf("Thread ID: %i, CLEARED NUMBER OF CARS\n", threadId);
//          for (int i = 0; i < TRAFFIC_LIGHTS; i++) {
//            numberOfCars[i] = 0;
//          }
//          numberOfCarsMutex.unlock();
//
//          consumerWaitingMutex.lock();
//          for (int i = 0; i < threadCount; i++) {
//            consumerWaiting[i] = false;
//          }
//          consumerWaitingMutex.unlock();
//        }
//      }
    }
  
  
  bool handleEmptySignalEntries(int threadId, int threadCount) {
//    if (producersCompleted) {
//      consumerWaitingMutex.lock();
//      consumerWaiting[threadId] = true;
//      consumerWaitingMutex.unlock();
//      return true;
//    }
    
    return false;
  }
  
};

void printNumberOfCars() {
  numberOfCarsMutex.lock();
  printf("0: %i, 1: %i, 2: %i\n", numberOfCars[0], numberOfCars[1], numberOfCars[2]);
  numberOfCarsMutex.unlock();
}

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
  
//  producersCompletedMutex.lock();
//  producersCompleted = true;
//  producersCompletedMutex.unlock();
  
  for (int threadId = 0; threadId < CONSUMERS_TO_USE; threadId++) {
    consumerThreads[threadId].join();
  }
  
  stopTimer();
  
  printf("Read CSV: %.3fs\n", durationBetweenTimers());
  
  return 0;
}
