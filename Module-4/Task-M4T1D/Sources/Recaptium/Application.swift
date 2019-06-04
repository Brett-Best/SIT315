//
//  Application.swift
//  Recaptium
//
//  Created by Brett Best on 1/6/19.
//

import Foundation
import CMPI

class Application {
  
  static let shared = Application()
  
  var startDate = Date()
  
  private init() {
    var argc = CommandLine.argc
    var argv: UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>? = CommandLine.unsafeArgv
    
    var provided: Int32 = 0
    MPI_Init_thread(&argc, &argv, Int32(MPI_THREAD_MULTIPLE), &provided)
    if provided != MPI_THREAD_MULTIPLE {
      fatalError()
    }
  }
  
  func run() throws {
    let environment = Environment(argc: CommandLine.argc, argv: CommandLine.arguments)
    environment.printEnvironmentInfo()
    
    startDate = Date()
    
    let mlModelBuilder = try CoreMLModelBuilder(dataset: "fruits-360", environment: environment)
    
    if environment.createModelsEnabled {
      try mlModelBuilder.configureFolderStructure()
      
      let dispatchSemaphore = DispatchSemaphore(value: 0)
      
      mlModelBuilder.build() { _ in
        dispatchSemaphore.signal()
      }
      
      dispatchSemaphore.wait()
    }
    
    MPI_Barrier(MPI_COMM_WORLD)
    
    if environment.worldRank == 0 {
      print("[PERF] Building / Evaluating ML Models: \(Date().timeIntervalSince(startDate))s")
    }
    
    if environment.analyseImagesEnabled {
      startDate = Date()
      
      let models = try mlModelBuilder.compileModels()
      let visionProcessor = try VisionProcessor(environment: environment, models: models)
      
      visionProcessor.processImages(at: mlModelBuilder.mlDataURL.appendingPathComponent(mlModelBuilder.recaptionImagessFolderName, isDirectory: true))
      
      MPI_Barrier(MPI_COMM_WORLD)
      
      if environment.worldRank == 0 {
        print("[PERF] Analysing images using ML models: \(Date().timeIntervalSince(startDate))")
      }
    }
  }
  
  func exit() {
    MPI_Finalize()
  }
}

