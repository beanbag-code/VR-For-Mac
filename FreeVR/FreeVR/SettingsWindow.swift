import SwiftUI
import AppKit

struct SettingsWindow {
    private static var window: NSWindow?

    static func show(settings: RendererSettings) {
        if let existingWindow = window {
            // If window exists but was closed, check if it's still valid
            if existingWindow.isVisible {
                existingWindow.makeKeyAndOrderFront(nil)
                return
            } else {
                window = nil // discard old reference
            }
        }

        // Create a new SwiftUI view
        let contentView = SlidersView(settings: settings)

        // Wrap in NSWindow
        let newWindow = NSWindow(
            contentRect: NSRect(x: 100, y: 100, width: 400, height: 400),
            styleMask: [.titled, .closable, .resizable],
            backing: .buffered,
            defer: false
        )
        newWindow.title = "VR Settings"
        newWindow.contentView = NSHostingView(rootView: contentView)
        newWindow.makeKeyAndOrderFront(nil)

        // Keep a reference so we can reopen
        window = newWindow

        // Handle window close to release reference
        NotificationCenter.default.addObserver(forName: NSWindow.willCloseNotification, object: newWindow, queue: nil) { _ in
            window = nil
        }
    }
}

