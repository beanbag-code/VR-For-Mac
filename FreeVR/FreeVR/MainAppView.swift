import SwiftUI
import MetalKit
import Combine

struct MainAppView: View {
    @ObservedObject var settings: RendererSettings
    @StateObject var captureManager = ScreenCaptureManager()

    var body: some View {
        MetalCaptureView(settings: settings, captureManager: captureManager)
            .frame(minWidth: 800, minHeight: 600)
            .task {
                do {
                    try await captureManager.startCapture()
                } catch {
                    print("Screen capture startup failed: \(error)")
                }
            }
    }
}


struct MetalCaptureView: NSViewRepresentable {
    @ObservedObject var settings: RendererSettings
    @ObservedObject var captureManager: ScreenCaptureManager

    func makeNSView(context: Context) -> MTKView {
        let mtkView = MTKView()
        
        guard let device = MTLCreateSystemDefaultDevice() else {
            fatalError("Metal initialization error")
        }
        mtkView.device = device
        
        let renderer = MetalRenderer(mtkView: mtkView, settings: settings)
        context.coordinator.renderer = renderer
        
        captureManager.onFirstFrameSize = { size in
            mtkView.drawableSize = size
        }
        context.coordinator.cancellable = captureManager.pixelBufferSubject
            .sink { (pixelBuffer, timestamp) in
                // Forward both pieces of data to the processing loop
                renderer.processAndQueueFrame(from: pixelBuffer, timestamp: timestamp)
                
                DispatchQueue.main.async {
                    mtkView.draw()
                }
            }

        return mtkView
    }

    func updateNSView(_ nsView: MTKView, context: Context) {
        context.coordinator.renderer?.settings = settings
    }

    func makeCoordinator() -> Coordinator {
        Coordinator()
    }

    class Coordinator {
        var renderer: MetalRenderer?
        var cancellable: AnyCancellable?
    }
}
