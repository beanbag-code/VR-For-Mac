import SwiftUI

@main
struct FreeVRApp: App {
    @StateObject var settings = RendererSettings()

    var body: some Scene {
        WindowGroup {
            MainAppView(settings: settings)
        }
        .commands {
            CommandGroup(after: .appSettings) {
                Button("Open VR Settings") {
                    SettingsWindow.show(settings: settings)
                }
                .keyboardShortcut("S", modifiers: [.shift]) // Shift+S works anywhere in app
            }
        }
    }
}

