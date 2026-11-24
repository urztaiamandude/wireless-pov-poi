// swift-tools-version: 5.9
import PackageDescription

// DO NOT MODIFY THIS FILE - managed by Capacitor CLI commands
let package = Package(
    name: "CapApp-SPM",
    platforms: [.iOS(.v15)],
    products: [
        .library(
            name: "CapApp-SPM",
            targets: ["CapApp-SPM"])
    ],
    dependencies: [
        .package(url: "https://github.com/ionic-team/capacitor-swift-pm.git", exact: "8.4.3"),
        .package(name: "CapacitorCamera", path: "../../../../../../node_modules/@capacitor/camera"),
        .package(name: "CapacitorHaptics", path: "../../../../../../node_modules/@capacitor/haptics"),
        .package(name: "CapacitorNetwork", path: "../../../../../../node_modules/@capacitor/network"),
        .package(name: "CapacitorPreferences", path: "../../../../../../node_modules/@capacitor/preferences"),
        .package(name: "CapacitorScreenOrientation", path: "../../../../../../node_modules/@capacitor/screen-orientation"),
        .package(name: "CapacitorShare", path: "../../../../../../node_modules/@capacitor/share"),
        .package(name: "CapacitorStatusBar", path: "../../../../../../node_modules/@capacitor/status-bar"),
        .package(name: "CapacitorToast", path: "../../../../../../node_modules/@capacitor/toast")
    ],
    targets: [
        .target(
            name: "CapApp-SPM",
            dependencies: [
                .product(name: "Capacitor", package: "capacitor-swift-pm"),
                .product(name: "Cordova", package: "capacitor-swift-pm"),
                .product(name: "CapacitorCamera", package: "CapacitorCamera"),
                .product(name: "CapacitorHaptics", package: "CapacitorHaptics"),
                .product(name: "CapacitorNetwork", package: "CapacitorNetwork"),
                .product(name: "CapacitorPreferences", package: "CapacitorPreferences"),
                .product(name: "CapacitorScreenOrientation", package: "CapacitorScreenOrientation"),
                .product(name: "CapacitorShare", package: "CapacitorShare"),
                .product(name: "CapacitorStatusBar", package: "CapacitorStatusBar"),
                .product(name: "CapacitorToast", package: "CapacitorToast")
            ]
        )
    ]
)
