{
    "targets": [{
        "target_name": "easy-control",
        "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")"
        ],
        "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
        "sources": [
            "src/main.cpp",
            "src/mouse.cpp",
            "src/keyboard.cpp",
            "src/controller.cpp",
            "src/screen.cpp",
        ],
        "cflags": [
            "-Wall",
            "-Wparentheses",
            "-Winline",
            "-Wbad-function-cast",
            "-Wdisabled-optimization"
        ],
        "conditions": [
            [
                "OS=='win'",
                {
                    "defines": ["IS_WINDOWS"],
                    "include_dirs": [
                        "<(module_root_dir)/src/vjoy_driver/inc"
                    ],
                    "libraries": [
                        "<(module_root_dir)/src/vjoy_driver/lib/x64/vJoyInterface.lib"
                    ]
                }
            ],
            [
                "OS == 'mac'",
                {
                    "defines": ["IS_MACOS"],
                    "include_dirs": [
                        "System/Library/Frameworks/CoreFoundation.Framework/Headers",
                        "System/Library/Frameworks/Carbon.Framework/Headers",
                        "System/Library/Frameworks/ApplicationServices.framework/Headers",
                        "System/Library/Frameworks/OpenGL.framework/Headers",
                    ],
                    "link_settings": {
                        "libraries": [
                            "-framework Carbon",
                            "-framework CoreFoundation",
                            "-framework ApplicationServices",
                            "-framework OpenGL"
                        ]
                    }
                }
            ],
            [
                "OS == 'linux'",
                {
                    "defines": ["IS_LINUX"],
                    "link_settings": {
                        "libraries": [
                            "-lpng",
                            "-lz",
                            "-lX11",
                            "-lXtst"
                        ]
                    }
                }
            ]
        ]
    }]
}