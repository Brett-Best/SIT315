//
//  ImageClassifierBuilder.swift
//  Recaptium
//
//  Created by Brett Best on 31/5/19.
//

import Foundation
import CreateML

class ImageClassifierBuilder {
  
  init(trainingDataURL: URL, validationDataURL: URL, writeURL: URL, evaluationURL: URL?) throws {
    let trainingData = MLImageClassifier.DataSource.labeledDirectories(at: trainingDataURL)
    let validationData: [String : [URL]]?

    let noValidationData = try FileManager.default.contentsOfDirectory(at: validationDataURL, includingPropertiesForKeys: nil).isEmpty

    if !noValidationData {
      validationData = try MLImageClassifier.DataSource.labeledDirectories(at: validationDataURL).labeledImages()
    } else {
      validationData = nil
    }

    let parameters = MLImageClassifier.ModelParameters(validationData: validationData, maxIterations: 200)
    let builder = try MLImageClassifier(trainingData: trainingData, parameters: parameters)
    
    try builder.write(to: writeURL, metadata: MLModelMetadata(author: "Recaptium", shortDescription: "", license: nil, version: "1.0", additional: nil))
    
    if let evaluationURL = evaluationURL {
      print(builder.evaluation(on: MLImageClassifier.DataSource.labeledDirectories(at: evaluationURL)))
    }
  }
  
}
