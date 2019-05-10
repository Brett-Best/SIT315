//
//  main.swift
//  Traffic Generator
//
//  Created by Brett Best on 10/5/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

import Foundation

let totalMeasurements = 12 * 24 * 7
let trafficSignals = 100
let maxNumberOfCars = 1000

let startDate = Date()
var currentDate: Date = startDate

print("timestamp,trafficSignal,numberOfCars")

(0..<totalMeasurements).forEach { measurement in
  (0..<trafficSignals).forEach { trafficSignal in
    let numberOfCars = Int.random(in: 0...maxNumberOfCars)
    print("\(Int(currentDate.timeIntervalSince1970)),\(trafficSignal),\(numberOfCars)")
  }
  currentDate = currentDate.addingTimeInterval(60*5)
}
