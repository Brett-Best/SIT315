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
    
    let workAllocator = try WorkAllocator(dataset: "Flowers", environment: environment)
    try workAllocator.configureFolderStructure()
    
    let dispatchSemaphore = DispatchSemaphore(value: 0)
    
    workAllocator.build() { _ in
      dispatchSemaphore.signal()
    }
    
    dispatchSemaphore.wait()
  }
  
  func exit() {
    MPI_Finalize()
  }
}
