//
//  main.swift
//  Traffic Generator
//
//  Created by Brett Best on 10/5/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

import Foundation

let totalMeasurements = 12 * 24 * 3 // 12 per hour, x 4 hours, x 3 days
let trafficSignals = 10
let maxNumberOfCars = 100000

let startDate = Date()
var currentDate: Date = startDate

var totalNumberOfCars = 0

print("timestamp,trafficSignal,numberOfCars,totalNumberOfCars")

(0..<totalMeasurements).forEach { measurement in
  (0..<trafficSignals).forEach { trafficSignal in
    let numberOfCars = Int.random(in: 0...maxNumberOfCars)
    totalNumberOfCars = totalNumberOfCars + numberOfCars
    print("\(Int(currentDate.timeIntervalSince1970)),\(trafficSignal),\(numberOfCars),\(totalNumberOfCars)")
  }
  currentDate = currentDate.addingTimeInterval(60*5)
}
