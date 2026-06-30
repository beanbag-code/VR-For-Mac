import Foundation
import ScreenCaptureKit
import CoreMedia
import CoreVideo
import Combine
import AppKit

final class ScreenCaptureManager: NSObject, ObservableObject, SCStreamOutput {
    
    // Send both the frame buffer and its original generation timestamp
    let pixelBufferSubject = PassthroughSubject<(CVPixelBuffer, CFTimeInterval), Never>()
    var onFirstFrameSize: ((CGSize) -> Void)?
    
    private var stream: SCStream?
    private var didSetDrawableSize = false
    
    func startCapture() async throws {
        let content = try await SCShareableContent.current
        
        guard let display = content.displays.first else {
            print("No displays found")
            return
        }
        
        // Exclude our own window to break recursive capture loop mechanics
        let currentAppPID = NSRunningApplication.current.processIdentifier
        let appsToExclude = content.applications.filter { $0.processID == currentAppPID }
        
        let filter = SCContentFilter(
            display: display,
            excludingApplications: appsToExclude,
            exceptingWindows: []
        )
        
        let config = SCStreamConfiguration()
        config.pixelFormat = kCVPixelFormatType_32BGRA
        config.width = 1920
        config.height = 1080
        config.queueDepth = 2
        config.scalesToFit = false
        config.showsCursor = true
        config.capturesAudio = false
        
        config.minimumFrameInterval = CMTime(value: 1, timescale: 60)
        config.colorSpaceName = CGColorSpace.sRGB
        
        let stream = SCStream(
            filter: filter,
            configuration: config,
            delegate: nil
        )
        
        try stream.addStreamOutput(
            self,
            type: .screen,
            sampleHandlerQueue: DispatchQueue(label: "com.freevr.capture.queue", qos: .userInteractive)
        )
        
        try await stream.startCapture()
        self.stream = stream
    }
    
    func stopCapture() {
        stream?.stopCapture()
        stream = nil
    }
    
    func stream(
        _ stream: SCStream,
        didOutputSampleBuffer sampleBuffer: CMSampleBuffer,
        of type: SCStreamOutputType
    ) {
        guard let attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, createIfNecessary: false) as? [[String: Any]],
              let attachment = attachments.first,
              let statusRaw = attachment[SCStreamFrameInfo.status.rawValue] as? Int,
              statusRaw == SCFrameStatus.complete.rawValue else {
            return
        }
        
        guard let pixelBuffer = sampleBuffer.imageBuffer else { return }
        
        // Use absolute current media time instead of the target presentation time
        let arrivalTime = CACurrentMediaTime()
        
        if !didSetDrawableSize {
            didSetDrawableSize = true
            let size = CGSize(width: CVPixelBufferGetWidth(pixelBuffer), height: CVPixelBufferGetHeight(pixelBuffer))
            DispatchQueue.main.async { self.onFirstFrameSize?(size) }
        }
        
        self.pixelBufferSubject.send((pixelBuffer, arrivalTime))
    }
}
