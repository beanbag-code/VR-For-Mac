import Metal
import MetalKit
import simd
import CoreVideo

// Structs matching shader layout definitions
struct StereoUniforms {
    var eyeSeparation: Float
    var cropCenter: SIMD2<Float>
    var cropScale: SIMD2<Float>
}

struct LensUniforms {
    var k1: Float
    var k2: Float
}

final class MetalRenderer: NSObject, MTKViewDelegate {
    
    private var nextFrameTimestamp: CFTimeInterval = 0
    private var activeFrameTimestamp: CFTimeInterval = 0
    
    let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private var pipelineState: MTLRenderPipelineState!
    private var vertexBuffer: MTLBuffer!
    private var samplerState: MTLSamplerState!
    
    private var textureCache: CVMetalTextureCache?
    
    // Using an atomic reference protection scheme
    private let queueLock = NSLock()
    private var nextTexture: MTLTexture?
    private var nextToken: CVMetalTexture?
    
    private var activeTexture: MTLTexture?
    private var activeToken: CVMetalTexture?

    var settings: RendererSettings

    init(mtkView: MTKView, settings: RendererSettings) {
        guard let device = mtkView.device else { fatalError("Metal device error") }
        self.device = device
        guard let queue = device.makeCommandQueue() else { fatalError("Command queue error") }
        self.commandQueue = queue
        self.settings = settings
        super.init()

        CVMetalTextureCacheCreate(kCFAllocatorDefault, nil, device, nil, &textureCache)

        mtkView.delegate = self
        mtkView.framebufferOnly = false
        mtkView.isPaused = true
        mtkView.enableSetNeedsDisplay = false

        buildPipeline(mtkView: mtkView)
        buildBuffers()
        buildSampler()
    }

    func processAndQueueFrame(from pixelBuffer: CVPixelBuffer, timestamp: CFTimeInterval) {
        guard let textureCache = textureCache else { return }
        
        let width = CVPixelBufferGetWidth(pixelBuffer)
        let height = CVPixelBufferGetHeight(pixelBuffer)
        
        var cvMetalTexture: CVMetalTexture?
        let status = CVMetalTextureCacheCreateTextureFromImage(
            kCFAllocatorDefault,
            textureCache,
            pixelBuffer,
            nil,
            .bgra8Unorm,
            width,
            height,
            0,
            &cvMetalTexture
        )
        
        if status == kCVReturnSuccess, let cvMetalTexture = cvMetalTexture,
               let metalTexture = CVMetalTextureGetTexture(cvMetalTexture) {
                
                queueLock.lock()
                self.nextTexture = metalTexture
                self.nextToken = cvMetalTexture
                self.nextFrameTimestamp = timestamp // Track the timestamp
                queueLock.unlock()
        }
    }

    private func buildPipeline(mtkView: MTKView) {
        guard let library = device.makeDefaultLibrary(),
              let vertexFunc = library.makeFunction(name: "vertex_main"),
              let fragmentFunc = library.makeFunction(name: "fragment_main") else {
            fatalError("Shader functions missing")
        }

        let pipelineDesc = MTLRenderPipelineDescriptor()
        pipelineDesc.vertexFunction = vertexFunc
        pipelineDesc.fragmentFunction = fragmentFunc
        pipelineDesc.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat

        pipelineState = try! device.makeRenderPipelineState(descriptor: pipelineDesc)
    }

    private func buildBuffers() {
        let vertices: [Float] = [
            -1, -1, 0, 1,   1, -1, 1, 1,  -1,  1, 0, 0,
            -1,  1, 0, 0,   1, -1, 1, 1,   1,  1, 1, 0
        ]
        vertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.size, options: [])
    }

    private func buildSampler() {
        let descriptor = MTLSamplerDescriptor()
        descriptor.minFilter = .nearest
        descriptor.magFilter = .nearest
        descriptor.sAddressMode = .clampToEdge
        descriptor.tAddressMode = .clampToEdge
        samplerState = device.makeSamplerState(descriptor: descriptor)
    }

    func draw(in view: MTKView) {
        queueLock.lock()
        if nextTexture != nil {
            self.activeTexture = self.nextTexture
            self.activeToken = self.nextToken
            self.activeFrameTimestamp = self.nextFrameTimestamp // Grab the timestamp for this draw
            self.nextTexture = nil
            self.nextToken = nil
        }
        queueLock.unlock()

        guard let drawable = view.currentDrawable,
              let textureToRender = activeTexture,
              let descriptor = view.currentRenderPassDescriptor,
              let commandBuffer = commandQueue.makeCommandBuffer(),
              let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: descriptor) else { return }
        
        encoder.setRenderPipelineState(pipelineState)
        encoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        encoder.setFragmentTexture(textureToRender, index: 0)
        encoder.setFragmentSamplerState(samplerState, index: 0)

        var stereo = StereoUniforms(
            eyeSeparation: settings.eyeSeparation,
            cropCenter: SIMD2<Float>(settings.cropCenterX, settings.cropCenterY),
            cropScale: SIMD2<Float>(settings.cropScaleX, settings.cropScaleY)
        )
        var lens = LensUniforms(k1: settings.k1, k2: settings.k2)
        
        encoder.setFragmentBytes(&stereo, length: MemoryLayout<StereoUniforms>.stride, index: 0)
        encoder.setFragmentBytes(&lens, length: MemoryLayout<LensUniforms>.stride, index: 1)

        encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6)
        encoder.endEncoding()
            
            commandBuffer.present(drawable)
            
            let tokenToRelease = activeToken
            let cacheToFlush = textureCache
            let frameCaptureTime = activeFrameTimestamp // Capture locally for the block
            
            commandBuffer.addCompletedHandler { _ in
                _ = tokenToRelease
                if let cache = cacheToFlush {
                    CVMetalTextureCacheFlush(cache, 0)
                }
                
                // CALCULATE LATENCY HERE:
                let gpuCompleteTime = CACurrentMediaTime()
                let totalLatencyMilliseconds = (gpuCompleteTime - frameCaptureTime) * 1000
                
                print(String(format: "Pipeline Latency: %.2f ms", totalLatencyMilliseconds))
            }
            
            commandBuffer.commit()
        }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}
}
