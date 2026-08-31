// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "ThemisDB",
    platforms: [
        .macOS(.v13),
        .iOS(.v16)
    ],
    products: [
        .library(
            name: "ThemisDB",
            targets: ["ThemisDB"]
        )
    ],
    targets: [
        .target(
            name: "ThemisDB",
            path: "Sources/ThemisDB"
        )
    ]
)
