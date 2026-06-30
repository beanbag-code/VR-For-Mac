import Foundation
import Combine
import simd

final class RendererSettings: ObservableObject {
    // Stereo / crop
    @Published var eyeSeparation: Float = 0.004
    @Published var cropCenterX: Float = 0.50
    @Published var cropCenterY: Float = 0.48
    @Published var cropScaleX: Float = 0.70
    @Published var cropScaleY: Float = 0.85

    // Lens distortion
    @Published var k1: Float = 0.22
    @Published var k2: Float = 0.05
}
