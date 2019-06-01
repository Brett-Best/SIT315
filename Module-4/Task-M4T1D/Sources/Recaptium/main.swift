import Foundation
import CMPI

do {
  try Application.shared.run()
} catch {
  fatalError(error.localizedDescription)
}

Application.shared.exit()
