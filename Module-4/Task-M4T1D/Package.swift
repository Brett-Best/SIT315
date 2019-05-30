// swift-tools-version:5.0
import PackageDescription

let package = Package(
  name: "Recaptium",
  platforms: [
    .macOS(.v10_14)
  ],
  products: [
    .executable(name: "Recaptium", targets: ["Recaptium"]),
  ],
  dependencies: [
    .package(url: "https://github.com/Brett-Best/COpenMPI.git", .branch("feature/swift-5")),
  ],
  targets: [
    .target(name: "Recaptium", dependencies: ["COpenMPI"])
  ]
)
