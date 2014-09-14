{
  "targets": [
    {
      "target_name": "vcdiff",
      "dependencies": [
        'open-vcdiff',
      ],
      "sources": [
        "src/vcd_decoder.cc",
        "src/vcd_decoder.h",
        "src/vcd_encoder.cc",
        "src/vcd_encoder.h",
        "src/vcd_hashed_dictionary.cc",
        "src/vcd_hashed_dictionary.h",
        "src/vcdiff.cc",
        "src/vcdiff.h",
      ],
      "conditions": [
        ['OS=="linux"', {
          'cflags_cc': ['-std=c++11'],
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS': ['-std=c++11', '-stdlib=libc++'],
            'OTHER_LDFLAGS': ['-stdlib=libc++'],
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
          },
        }],
      ],
    },
  ],
  'includes': [ 'src/third-party/open-vcdiff.gypi' ],
}
