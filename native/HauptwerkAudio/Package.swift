// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "HauptwerkAudio",
    platforms: [
        .macOS(.v14),
        .macCatalyst(.v15)
    ],
    products: [
        .library(
            name: "HauptwerkAudio",
            type: .dynamic,
            targets: ["HauptwerkAudio"]
        )
    ],
    targets: [
        .target(
            name: "HauptwerkAudio",
            path: "Sources/HauptwerkAudio"
        )
    ]
)
