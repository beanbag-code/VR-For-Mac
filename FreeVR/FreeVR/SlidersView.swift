import SwiftUI

struct SlidersView: View {
    @ObservedObject var settings: RendererSettings

    var body: some View {
        VStack(spacing: 12) {
            
            // MARK: Presets
            HStack(spacing: 8) {
                Button("Default") { applyDefaultPreset() }
                Button("Cockpit Wide") { applyCockpitWidePreset() }
                Button("Focus Instruments") { applyInstrumentsPreset() }
            }
            .padding(4)

            Divider()

            // MARK: Stereo / Crop
            GroupBox("Stereo / Crop") {
                VStack(spacing: 4) {
                    SliderRow(label: "Eye Sep", value: $settings.eyeSeparation, range: 0...0.01)
                    SliderRow(label: "Crop X", value: $settings.cropCenterX, range: 0...1)
                    SliderRow(label: "Crop Y", value: $settings.cropCenterY, range: 0...1)
                    SliderRow(label: "Scale X", value: $settings.cropScaleX, range: 0.1...1)
                    SliderRow(label: "Scale Y", value: $settings.cropScaleY, range: 0.1...1)
                }
                .padding(4)
            }

            // MARK: Lens Distortion
            GroupBox("Lens Distortion") {
                VStack(spacing: 4) {
                    SliderRow(label: "k1", value: $settings.k1, range: 0...0.5)
                    SliderRow(label: "k2", value: $settings.k2, range: 0...0.2)
                }
                .padding(4)
            }
        }
        .padding()
    }

    // MARK: SliderRow subview
    struct SliderRow: View {
        let label: String
        @Binding var value: Float
        let range: ClosedRange<Float>

        var body: some View {
            HStack {
                Text(label).frame(width: 80, alignment: .leading)
                Slider(value: $value, in: range)
                Text(String(format: "%.3f", value)).frame(width: 50, alignment: .trailing)
            }
        }
    }

    // MARK: Preset functions
    func applyDefaultPreset() {
        settings.eyeSeparation = 0.004
        settings.cropCenterX = 0.50
        settings.cropCenterY = 0.48
        settings.cropScaleX = 0.70
        settings.cropScaleY = 0.85
        settings.k1 = 0.22
        settings.k2 = 0.05
    }

    func applyCockpitWidePreset() {
        settings.eyeSeparation = 0.005
        settings.cropCenterX = 0.50
        settings.cropCenterY = 0.50
        settings.cropScaleX = 0.85
        settings.cropScaleY = 0.90
        settings.k1 = 0.18
        settings.k2 = 0.04
    }

    func applyInstrumentsPreset() {
        settings.eyeSeparation = 0.0035
        settings.cropCenterX = 0.50
        settings.cropCenterY = 0.48
        settings.cropScaleX = 0.65
        settings.cropScaleY = 0.80
        settings.k1 = 0.22
        settings.k2 = 0.05
    }
}
