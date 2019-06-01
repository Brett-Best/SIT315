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
    
    MPI_Init(&argc, &argv)
  }
  
  func run() throws {
    let environment = Environment(argc: CommandLine.argc, argv: CommandLine.arguments)
    environment.printEnvironmentInfo()
    
    if environment.createModelsEnabled {
      let mlModelBuilder = try CoreMLModelBuilder(dataset: "Animals", environment: environment)
      try mlModelBuilder.configureFolderStructure()
      
      let dispatchSemaphore = DispatchSemaphore(value: 0)
      
      mlModelBuilder.build() { _ in
        dispatchSemaphore.signal()
      }
      
      dispatchSemaphore.wait()
    }
    
    if environment.analyseImagesEnabled {
      let visionProcessor = VisionProcessor(environment: environment)
      visionProcessor.compileModels()
    }
  }
  
  func exit() {
    MPI_Finalize()
  }
}

