{
  "targets": [
    {
      "target_name": "hpxaddon",
      "sources": [
        "src/addon/addon.cpp",
        "src/hpx_wrapper/hpx_wrapper.cpp",
        "src/hpx_manager/hpx_manager.cpp",
        "src/hpx_config/hpx_config.cpp",
        "src/utils/async_helpers.cpp",
        "src/utils/data_conversion.cpp",
        "src/utils/tsfn_manager.cpp",
        "src/logging/logger.cpp",
        "src/logging/log.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "/usr/local/hpx/include",
        "src/logging",
        "src/utils",
        "src/addon",
        "src/hpx_wrapper",
        "src/hpx_manager",
        "src/hpx_config",
        "src/extern/json/include"
      ],
      "libraries": [
        "-L/usr/local/hpx/lib",
        "-lhpx",
        "-ljemalloc",
        "-lhwloc",
        "-lpthread"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_EXPERIMENTAL"
      ],
      "cflags_cc!": [
        "-fno-exceptions",
        "-fno-strict-aliasing"
      ],
      "cflags_cc": [
        "-std=c++17",
        "-frtti"
      ],
      "ldflags": [
      ],
      "conditions": [
        [
          "OS=='linux'",
          {
            "link_settings": {
              "ldflags": [
                "-rdynamic"
              ]
            }
          }
        ]
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ]
    }
  ]
}
