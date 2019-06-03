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
  
  private init() {
    var argc = CommandLine.argc
    var argv: UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>? = CommandLine.unsafeArgv
    
//    MPI_Init(&argc, &argv)
    var provided: Int32 = 0
    MPI_Init_thread(&argc, &argv, Int32(MPI_THREAD_MULTIPLE), &provided)
    if provided != MPI_THREAD_MULTIPLE {
      fatalError()
    }
  }
  
  func run() throws {
    let environment = Environment(argc: CommandLine.argc, argv: CommandLine.arguments)
    environment.printEnvironmentInfo()
    
    let mlModelBuilder = try CoreMLModelBuilder(dataset: "Animals", environment: environment)
    
    if environment.createModelsEnabled {
      try mlModelBuilder.configureFolderStructure()
      
      let dispatchSemaphore = DispatchSemaphore(value: 0)
      
      mlModelBuilder.build() { _ in
        dispatchSemaphore.signal()
      }
      
      dispatchSemaphore.wait()
    }
    
    MPI_Barrier(MPI_COMM_WORLD)
    
    if environment.analyseImagesEnabled {
      let models = try mlModelBuilder.compileModels()
      let visionProcessor = try VisionProcessor(environment: environment, models: models)
      
      visionProcessor.processImages(at: mlModelBuilder.mlDataURL.appendingPathComponent(mlModelBuilder.recaptionImagessFolderName, isDirectory: true)) {}
    }
  }
  
  func exit() {
    MPI_Finalize()
  }
}

