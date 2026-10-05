{
    "targets": [{
        "target_name": "easy-control",
        "conditions": [
            [
                "OS=='win'",
                {
                    "defines": ["IS_WINDOWS", "NAPI_DISABLE_CPP_EXCEPTIONS"],
                    "msvs_settings": {
                        "VCCLCompilerTool": {
                            "WarningLevel": "3"
                        }
                    },
                    "sources": [
                        "src/native/main.cpp",
                        "src/native/mouse.cpp",
                        "src/native/keyboard.cpp",
                        "src/native/gamepad.cpp",
                        "src/native/screen.cpp",
                    ],
                    "include_dirs": [
                        "<!@(node -p \"require('node-addon-api').include\")",
                        "<(module_root_dir)/src/native/inc/"
                    ],
                    "libraries": [
                        "advapi32.lib",
                        "shcore.lib",
                        "<(module_root_dir)/src/native/inc/ViGEm/lib/ViGEmClient.lib"
                    ]
                }
            ],
            [
                "OS == 'mac'",
                {
                    "defines": ["IS_MACOS", "NAPI_DISABLE_CPP_EXCEPTIONS"],
                    "actions": [
                        {
                            "action_name": "build_swift",
                            "inputs": [
                                "src/native/GamepadImplement.swift"
                            ],
                            "outputs": [
                                "build_swift/libGamepadImplement.a",
                                "build_swift/gamepad_implement-Swift.h"
                            ],
                            "action": [
                                "swiftc",
                                "src/native/GamepadImplement.swift",
                                "-parse-as-library",
                                "-emit-objc-header-path", "./build_swift/gamepad_implement-Swift.h",
                                "-emit-library", "-static", "-o", "./build_swift/libGamepadImplement.a",
                                "-module-name", "gamepad_implement"
                            ]
                        }
                    ],
                    "sources": [
                        "src/native/main.cpp",
                        "src/native/mouse.cpp",
                        "src/native/keyboard.cpp",
                        "src/native/gamepad.cpp",
                        "src/native/screen.cpp",
                        "src/native/GamepadBridge.m"
                    ],
                    "include_dirs": [
                        "<!@(node -p \"require('node-addon-api').include\")",
                        "src/native",
                        "build_swift"
                    ],
                    "libraries": [
                        "<(module_root_dir)/build_swift/libGamepadImplement.a"
                    ],
                    "xcode_settings": {
                        "CLANG_ENABLE_OBJC_ARC": "YES",
                        "MACOSX_DEPLOYMENT_TARGET": "10.15",
                        "OTHER_CFLAGS": [
                            "-fobjc-arc"
                        ],
                        "OTHER_CPLUSPLUSFLAGS": [
                            "-ObjC++",
                            "-fobjc-arc"
                        ],
                        "OTHER_LDFLAGS": [
                            "-L/usr/lib/swift",
                            "-L<!(xcrun --show-sdk-path)/usr/lib/swift",
                            "-Wl,-rpath,/usr/lib/swift",
                            "-framework AppKit",
                            "-framework Carbon",
                            "-framework CoreGraphics",
                            "-framework IOKit",
                            "-framework Foundation",
                            "-weak_framework CoreHID"
                        ]
                    }
                }
            ],
            [
                "OS == 'linux'",
                {
                    "defines": ["IS_LINUX", "NAPI_DISABLE_CPP_EXCEPTIONS"],
                    "cflags": [
                        "-Wall",
                        "-Wparentheses",
                        "-Winline",
                        "-Wbad-function-cast",
                        "-Wdisabled-optimization"
                    ],
                    "sources": [
                        "src/native/main.cpp",
                        "src/native/mouse.cpp",
                        "src/native/keyboard.cpp",
                        "src/native/gamepad.cpp",
                        "src/native/screen.cpp",
                        "src/native/uinput.cpp",
                        "src/native/wayland.cpp"
                    ],
                    "include_dirs": [
                        "<!@(node -p \"require('node-addon-api').include\")"
                    ],
                    "link_settings": {
                        "libraries": [
                            "-lX11",
                            "-lXtst",
                            "-lXfixes",
                            "-lXrandr",
                            "-ldl"
                        ]
                    }
                }
            ]
        ]
    }]
}
