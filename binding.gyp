{
  "targets": [{
    "target_name": "pigpiod",
    "conditions": [[
      "OS == \"linux\"", {
        "cflags": [
          "-Wno-unused-local-typedefs"
        ]
      }]
    ],
    "include_dirs" : [
      "<!(node -e \"require('nan')\")"
    ],
    "sources": [
      "./src/pigpiod.cc"
    ],
    "link_settings": {
      "libraries": [
        "-lpigpiod_if2"
      ]
    }
  }]
}

