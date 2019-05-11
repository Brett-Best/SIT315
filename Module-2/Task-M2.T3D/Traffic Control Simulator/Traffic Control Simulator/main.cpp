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
#include <map>
#include <math.h>

using namespace std;

#define PRODUCERS_TO_USE 4
#define CONSUMERS_TO_USE 4
#define READ_BUFFER_SIZE 5 // Max number of entries to have in memory
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

struct TrafficSignalEntry {
  long long timestamp;
  int id;
  int numberOfCars;
};

struct TrafficSignalMetric {
  long long startTimeStamp;
  long long endTimeStamp;
  map<int, int> trafficSignalToCarsMap;
  
  void sort() {
    
  }
};

TrafficSignalEntry firstTrafficSignalEntry;
vector<TrafficSignalEntry> trafficSignalEntries;
int trafficSignalEntriesRead = 0;

int producersCompleted = 0;
mutex producersCompletedMutex;

mutex trafficSignalEntriesReadMutex;

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
      if (line == 0) firstTrafficSignalEntry = trafficSignalEntry;
      
      bool processed = false;
      
      while (!processed) {
        trafficSignalEntriesReadMutex.lock();
        
        if (trafficSignalEntriesRead == line) {
          unsigned long size = trafficSignalEntries.size();
          if (size >= READ_BUFFER_SIZE) {
            trafficSignalEntriesReadMutex.unlock();
            printf("DELAYED - Thread Id: %i, Timestamp: %lli, Id: %i, Number of cars: %i, Size: %lu\n", threadId, trafficSignalEntry.timestamp, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, size);
            continue;
          }
          
          processed = true;
          trafficSignalEntriesRead++;
          trafficSignalEntries.push_back(trafficSignalEntry);
          size = trafficSignalEntries.size();
          printf("PROCESSED - Thread Id: %i, Timestamp: %lli, Id: %i, Number of cars: %i, Size: %lu\n", threadId, trafficSignalEntry.timestamp, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, size);
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

vector<TrafficSignalMetric> trafficSignalMetrics;
mutex trafficSignalMetricsMutex;



class Consumer {
  
  long long maxTime = 0;
  TrafficSignalEntry lastTrafficSignalEntry;
  
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
      trafficSignalEntriesReadMutex.unlock();
      return handleEmptySignalEntries(threadId, threadCount);
    }

    TrafficSignalEntry trafficSignalEntry = trafficSignalEntries[0];
    trafficSignalEntries.erase(trafficSignalEntries.begin());
    
    unsigned long size = trafficSignalEntries.size();
    createNewTrafficMetricIfNeeded(trafficSignalEntry);
    updateTrafficMetric(trafficSignalEntry);
    printf("CONSUMED - Thread Id: %i, Timestamp: %lli, Id: %i, Number of cars: %i, Total number of cars: %i, Size: %lu\n", threadId, trafficSignalEntry.timestamp, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, 0, size);
    trafficSignalEntriesReadMutex.unlock();
    
    return false;
    }
  
  
  bool handleEmptySignalEntries(int threadId, int threadCount) {
    producersCompletedMutex.lock();
    
    if (producersCompleted == PRODUCERS_TO_USE) {
      producersCompletedMutex.unlock();
      return true;
    }
    
    producersCompletedMutex.unlock();
    
    return false;
  }
  
  void createNewTrafficMetricIfNeeded(TrafficSignalEntry trafficSignalEntry) {
    long long index = indexForTrafficSignalEntry(trafficSignalEntry);
    
    trafficSignalMetricsMutex.lock();
    if (trafficSignalMetrics.size() <= index) {
      long long startTimeStamp = firstTrafficSignalEntry.timestamp + 60*60*index;
      long long endTimeStamp = startTimeStamp + 60*60;
      trafficSignalMetrics.push_back(TrafficSignalMetric{ startTimeStamp, endTimeStamp });
      
      TrafficSignalMetric metric = trafficSignalMetrics[index];
      printf("METRIC CREATED - Start: %llu, End: %llu, Map Count: %lu, Metric Count: %lu\n", metric.startTimeStamp, metric.endTimeStamp, metric.trafficSignalToCarsMap.size(), trafficSignalMetrics.size());
    }
    trafficSignalMetricsMutex.unlock();
  }
  
  void updateTrafficMetric(TrafficSignalEntry trafficSignalEntry) {
    long long index = indexForTrafficSignalEntry(trafficSignalEntry);
    trafficSignalMetricsMutex.lock();
    TrafficSignalMetric *trafficSignalMetric = &trafficSignalMetrics[index];
    trafficSignalMetric->trafficSignalToCarsMap[trafficSignalEntry.id] += trafficSignalEntry.numberOfCars;
    printf("METRIC UPDATED - Count: ");
    for (const auto &pair : trafficSignalMetrics[index].trafficSignalToCarsMap) {
      printf("map[%i] = %i, ", pair.first, pair.second);
    }
    printf("\n");
    trafficSignalMetricsMutex.unlock();
  }
  
  long long indexForTrafficSignalEntry(TrafficSignalEntry trafficSignalEntry) {
    long long offset = trafficSignalEntry.timestamp-firstTrafficSignalEntry.timestamp;
    return (long long)floor(offset/60.0/60.0);
  }
  
};

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
  
  for (int threadId = 0; threadId < CONSUMERS_TO_USE; threadId++) {
    consumerThreads[threadId].join();
  }
  
  stopTimer();
  
  printf("Read CSV: %.3fs\n", durationBetweenTimers());
  
  return 0;
}
