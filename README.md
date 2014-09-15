# node-vcdiff

[![Build Status](https://travis-ci.org/baranov1ch/node-vcdiff.svg?branch=master)](https://travis-ci.org/baranov1ch/node-vcdiff)

node.js wrapper for [open-vcdiff](http://code.google.com/p/open-vcdiff)

## Notes

If you want to create SDCH-aware server, better look at [node-sdch](http://github.com/baranov1ch/node-sdch) and [connect-sdch](http://github.com/baranov1ch/connect-sdch)
which use this library internally and serve SDCH-encoded content which could
be comprehended by Chromium-based browsers.

You can use [femtozip](http://github.com/gtoubassi/femtozip) to create dictionaries.

## Quick Examples

```javascript
var vcd = require('vcdiff');

var dictionary = new Buffer(
  'Yo dawg I heard you like common substrings in your documents so we put ' +
  'them in your vcdiff dictionary so you can compress while you compress');

var hashed = new vcd.HashedDictionary(dictionary);

var testData =
  'Yo dawg I heard you like common substrings somewhere else so we put ' +
  'them in your vcdiff dictionary so you can decompress while you decompress'
```

Sync API

```javascript

var encoded = vcd.vcdiffEncodeSync(testData, { hashedDictionary: hashed });

var decoded = vcd.vcdiffDecodeSync(encoded, { dictionary: dictionary });

assert(testData === decoded.toString())

```

Async API

```javascript
vcd.vcdiffEncode(
  testData, { hashedDictionary: hashed }, function(err, enc) {
  vcd.vcdiffDecode(enc, { dictionary: dictionary }, function(err, dec) {
    assert(testData === dec.toString());
  });
});

```

Stream API

```javascript
var in = createInputStreamSomehow();
var out = createOutputStreamSomehow();
var encoder = vcd.createVcdiffEncoder({ hashedDictionary: hashed });
in.pipe(encoder).pipe(out);

var decoder = vcd.createVcdiffDecoder({ dictionary: dictionary });
out.pipe(decoder).pipe(process.stdout);
```

## Install

  `npm install vcdiff`

The source is available for download from
[GitHub](http://github.com/baranov1ch/node-vcdiff)

## TODO

#### Implement zero-copy encoding/decoding.

open-vcdiff outputs data to `OutputStringInterface`. It should behave as
`std::string` (i.e. be able to grow itself). Thus we cannot simply provide
output node Buffer. Now open-vcdiff outputs into `std::string` which then
copied into node Buffer. This is clearly suboptimal.

## License

[MIT](LICENSE)

open-vcdiff is distributed under [Apache](http://www.apache.org/licenses/LICENSE-2.0) license.

parts of the chromium code are distributed under [chromium BSD-style license](https://code.google.com/p/chromium/codesearch#chromium/src/LICENSE)
