# node-vcdiff

[![Build Status](https://travis-ci.org/baranov1ch/node-vcdiff.svg?branch=master)](https://travis-ci.org/baranov1ch/node-vcdiff)

node.js wrapper for [open-vcdiff](http://code.google.com/p/open-vcdiff)

## Notes

If you want to create SDCH-aware server, better look at [node-sdch](http://github.com/baranov1ch/node-sdch) and [connect-sdch](http://github.com/baranov1ch/connect-sdch)
which use this library internally and serve SDCH-encoded content which could
be comprehended by Chromium-based browsers.

You can use [femtozip](http://github.com/gtoubassi/femtozip) or [experimental incremental generator](https://github.com/cscenter/SInGe/tree/master/src/dict_builder) to create dictionaries.
Femtozip took quite a long time to generate (works for minutes and hours) dictionary. SInGe generator is much faster (seconds) and the same volume, since it never allocates during generation and uses linear algorithm based on suffix automaton. Quality is similar, since algorithm is essentially the same.

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

Make sure your compiler supports C++ 11. g++-4.8 will do, as well as clang
bundled with Xcode 5.

The source is available for download from
[GitHub](http://github.com/baranov1ch/node-vcdiff)

## API Reference

Just like standard zlib module, vcdiff provides three options to use it:
* sync
  `vcdiffEncodeSync(data, opts)` and `vcdiffDecodeSync(data, opts)`
* async
  `vcdiffEncode(data, opts, callback)` and `vcdiffDecode(data, opts, callback)`
* streaming
  create stream (`createVcdiffEncoder(opts)` or just `new VcdiffEncoder(opts)`)
  and pipe data through it.
  Same for decode (`createVcdiffDecoder(opts)` or `new VcdiffDecoder(opts)`)

`data` should be `string` or `Buffer`

`opts` dictionary of options. Differ for encode and decode. See below.

`callback` just a `function(error, data)`

### Options

In any case you should provide dictionary for encoding/decoding. Because of the
way Vcdiff works dictionaries for encode and decode differ. If you're not
intersted in all this and just want to server SDCH-encoded content, skip this
and use [node-sdch](http://github.com/baranov1ch/node-sdch).

#### Encoding

The encoding options are:

##### hashedDictionary

instance of `vcdiff.HashedDictionary`

You should provide at least this field to construct Vcdiff encoder.

Be sure not to create `HashedDictionary` on every encoding attempt,
since it copies the content of the provided buffer. This is the way open-vcdiff
works. Optimally, you should create `HashedDictionary` once and use it
everywhere in your app. The underlying open-vcdiff class is thread-safe and so
on.

`HashedDictionary` is constructed from a `Buffer` containing dictionary data
as simple as:
```javascript
var dictionary = new Buffer('this is my dictionary for vcdiff compression');
var hd = new vcdiff.HashedDictionary(dictionary);
```


##### minEncodeWindowSize

`Number`, minimum - 60, maximum - Infinity, default - 4096.

minimal size of the data chunk (in bytes) that will be
passed to encoder unless you flush the encoder or data is shorter.

The more data you provide to the encoder, the longer substrings it could find,
the more efficient encoding can be. But always think about the stream
responsiveness.

##### targetMatches

`Boolean`, default - false.

From open-vcdiff docs:
Find duplicate strings in target data as well as dictionary data.


The following flags change output of the encoder to non-stadard vcdiff. Be sure
to decode it with open-vcdiff as well.

From open-vcdiff docs:
These flags are passed to the constructor of VCDiffStreamingEncoder
to determine whether certain open-vcdiff format extensions
(which are not part of the RFC 3284 draft standard for VCDIFF)
are employed.

Because these extensions are not part of the VCDIFF standard, if
any of these flags except VCD_STANDARD_FORMAT is specified, then the caller
must be certain that the receiver of the data will be using open-vcdiff
to decode the delta file, or at least that the receiver can interpret
these extensions.  The encoder will use an 'S' as the fourth character
in the delta file to indicate that non-standard extensions are being used.

##### interleaved

`Boolean`, default - false.

From open-vcdiff docs:
If this flag is specified, then the encoder writes each delta file
window by interleaving instructions and sizes with their corresponding
addresses and data, rather than placing these elements
into three separate sections.  This facilitates providing partially
decoded results when only a portion of a delta file window is received
(e.g. when HTTP over TCP is used as the transmission protocol.)

##### checksum

`Boolean`, default - false.

From open-vcdiff docs:
If this flag is specified, then an Adler32 checksum
of the target window data is included in the delta window.

##### json

`Boolean`, default - false.

From open-vcdiff docs:
If this flag is specified, the encoder will output a JSON string
instead of the VCDIFF file format. If this flag is set, all other
flags have no effect.

#### Decoding

The decoding options are:

##### dictionary

instance of `Buffer`.

You should provide at least this field to construct Vcdiff decoder.

The contents of the buffer is not copied but `v8::Persistent` reference to this
buffer is kept for the existence of the decoder.

##### allowVcdTarget

`Boolean`, default - true.

From open-vcdiff docs:
If its argument is true, then the VCD_TARGET flag can be specified to allow the
source segment to be chosen from the previously-decoded target data.  (This is
the default behavior.)  If it is false, then specifying the VCD_TARGET flag is
considered an error, and the decoder does not need to keep in memory any
decoded target data prior to the current window.

##### maxTargetFileSize

`Number`, minimum - 1, maximum - `1 << 32 - 1`, default - `1 << 26` (64Mb).

From open-vcdiff docs:
Specifies the maximum allowable target file size.  If the decoder
encounters a delta file that would cause it to create a target file larger
than this limit, it will log an error and stop decoding.  If the decoder is
applied to delta files whose sizes vary greatly and whose contents can be
trusted, then a value larger than the the default value (64 MB) can be
specified to allow for maximum flexibility.  On the other hand, if the
input data is known never to exceed a particular size, and/or the input
data may be maliciously constructed, a lower value can be supplied in order
to guard against running out of memory or swapping to disk while decoding
an extremely large target file.  The argument must be between 0 and
INT32_MAX (2G); if it is within these bounds, the function will set the
limit and return true.  Otherwise, the function will return false and will
not change the limit.  Setting the limit to 0 will cause all decode
operations of non-empty target files to fail.

##### maxTargetWindowSize

`Number`, minimum - 1, maximum - `1 << 32 - 1`, default - `1 << 26` (64Mb).

From open-vcdiff docs:
Specifies the maximum allowable target *window* size.  (A target file is
composed of zero or more target windows.)  If the decoder encounters a
delta window that would cause it to create a target window larger
than this limit, it will log an error and stop decoding.

## TODO

#### Get rid of excessive copies in encoding/decoding process.

open-vcdiff outputs data to `OutputStringInterface`. It should behave as
`std::string` (i.e. be able to grow itself). Thus we cannot simply provide
output node Buffer. Now open-vcdiff outputs into `std::string` which then
copied into node Buffer. This is clearly suboptimal.

## License

[MIT](LICENSE)

open-vcdiff is distributed under [Apache](http://www.apache.org/licenses/LICENSE-2.0) license.

parts of the chromium code are distributed under [chromium BSD-style license](https://code.google.com/p/chromium/codesearch#chromium/src/LICENSE)
