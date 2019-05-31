//
//  ImageClassifierBuilder.swift
//  Recaptium
//
//  Created by Brett Best on 31/5/19.
//

import Foundation
import CreateML

class ImageClassifierBuilder {
  
  init(trainingDataURL: URL) throws {
    let dataSource = MLImageClassifier.DataSource.labeledDirectories(at: trainingDataURL)
    let parameters = MLImageClassifier.ModelParameters(maxIterations: 200)
    let builder = try MLImageClassifier(trainingData: dataSource, parameters: parameters)
  }
  
}
