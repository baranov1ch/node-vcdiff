vcd = require '../lib/vcdiff'
chai = require 'chai'
stream = require 'stream'

chai.should()

describe 'vcdiff', ->
  it 'should have all expected exports', ->
    vcd.should.respondTo 'HashedDictionary'
    vcd.should.respondTo 'VcdiffEncoder'
    vcd.should.respondTo 'VcdiffDecoder'
    vcd.should.respondTo 'createVcdiffEncoder'
    vcd.should.respondTo 'createVcdiffDecoder'
    vcd.should.respondTo 'vcdiffEncode'
    vcd.should.respondTo 'vcdiffEncodeSync'
    vcd.should.respondTo 'vcdiffDecode'
    vcd.should.respondTo 'vcdiffDecodeSync'
    vcd.should.have.property 'codes'
    vcd.codes.should.have.property 'VCD_INIT_ERROR'
    vcd.codes.should.have.property 'VCD_ENCODE_ERROR'
    vcd.codes.should.have.property 'VCD_DECODE_ERROR'
    vcd.codes.should.have.property '1'
    vcd.codes.should.have.property '2'
    vcd.codes.should.have.property '3'
    vcd.codes.VCD_INIT_ERROR.should.equal 1
    vcd.codes.VCD_ENCODE_ERROR.should.equal 2
    vcd.codes.VCD_DECODE_ERROR.should.equal 3
    vcd.codes[1].should.equal 'VCD_INIT_ERROR'
    vcd.codes[2].should.equal 'VCD_ENCODE_ERROR'
    vcd.codes[3].should.equal 'VCD_DECODE_ERROR'

  describe 'VcdiffEncoder', ->
    it 'should throw if no options provided', ->
      vcd.createVcdiffEncoder.should.throw Error, /HashedDictionary/
      vcd.VcdiffEncoder.should.throw Error, /HashedDictionary/
      vcd.vcdiffEncode.should.throw Error, /HashedDictionary/

    it 'should throw if options does not contain hashed dictionary', ->
      (-> vcd.createVcdiffEncoder allowVcdTarget: true)
        .should.throw Error, /HashedDictionary/
      (-> vcd.VcdiffEncoder allowVcdTarget: true)
        .should.throw Error, /HashedDictionary/
      (-> vcd.vcdiffEncode allowVcdTarget: true)
        .should.throw Error, /HashedDictionary/

    it 'should throw if dictionary is not Object', ->
      # Figure out how to test if its really HashedDictionary instance.
      (-> vcd.createVcdiffEncoder hashedDictionary: true)
        .should.throw Error, /HashedDictionary/
      (-> vcd.VcdiffEncoder hashedDictionary: true)
        .should.throw Error, /HashedDictionary/
      (-> vcd.vcdiffEncode hashedDictionary: true)
        .should.throw Error, /HashedDictionary/

    it 'should be created if everything is OK', ->
      hashedDict = new vcd.HashedDictionary new Buffer 'test'
      encoder = vcd.createVcdiffEncoder hashedDictionary: hashedDict
      encoder.should.be.instanceof vcd.VcdiffEncoder

    it 'should respond to public interface methods', ->
      hashedDict = new vcd.HashedDictionary new Buffer 'test'
      encoder = vcd.createVcdiffEncoder hashedDictionary: hashedDict
      encoder.should.respondTo 'flush'
      encoder.should.respondTo 'close'

    it 'should accept flags', ->
      encoder = vcd.createVcdiffEncoder
        hashedDictionary: new vcd.HashedDictionary new Buffer 'test'
        interleaved: true
      encoder.should.be.instanceof vcd.VcdiffEncoder

    describe 'options handling', ->
      dict = new Buffer 'this is a test dictionary not very long'
      testData = new Buffer(
        'this is a test dictionary not very long a test dictionary not')
      it 'should encode json for ascii', ->
        e = vcd.vcdiffEncodeSync(
          testData
          hashedDictionary: new vcd.HashedDictionary dict
          json: true)
        (-> JSON.parse e.toString())
          .should.not.throw

      it 'should encode interleaved', ->
        e = vcd.vcdiffEncodeSync(
          testData
          hashedDictionary: new vcd.HashedDictionary dict
          interleaved: true)
        # extended Vcdiff format. Header should have 'S' flag.
        e.toString()[3].should.equal 'S'

      it 'should encode with checksum', ->
        withoutChecksum = vcd.vcdiffEncodeSync(
          testData
          hashedDictionary: new vcd.HashedDictionary dict)
        withChecksum = vcd.vcdiffEncodeSync(
          testData
          hashedDictionary: new vcd.HashedDictionary dict
          checksum: true)
        withChecksum.toString()[3].should.equal "S"
        withChecksum.length.should.be.above withoutChecksum.length

      xit 'should set targetMatches', ->
        # No idea how to test it yet. Perhaps, use spies.

      xit 'should return error if input invalid', ->
        # No idea how to mangle the data to stop being valid for encoding.

    describe 'buffering', ->
      dict = new Buffer 'this is a test dictionary not very long'
      testData = 'this is a test dictionary not very long plus smth'
      hashedDict = new vcd.HashedDictionary dict

      class RepeatableRead extends stream.Readable
        constructor: (@chunk, @times) ->
          @currentChunk = 0
          super()

        _read: ->
          if @currentChunk < @times
            @push @chunk
            @currentChunk += 1
          else
            @push null

      class Flush extends stream.Transform
        constructor: (@readcb) ->
          super()

        _transform: (chunk, enc, next) ->
          @push chunk
          self = @
          process.nextTick ->
            self.readcb()
            next()

      class CountingWrite extends stream.Writable
        constructor: (finishcb) ->
          super()
          @nwrites = 0
          @on 'finish', ->
            finishcb(@nwrites)

        _write: (chunk, encoding, next) ->
          @nwrites += 1
          next()

      it 'should buffer input', (done) ->
        encoder = vcd.createVcdiffEncoder
          hashedDictionary: hashedDict
          minEncodeWindowSize: testData.length * 2
        inp = new RepeatableRead testData, 6
        out = new CountingWrite (nwrites) ->
          nwrites.should.equal 3
          done()
        inp.pipe(encoder).pipe(out)

      it 'should flush if asked', (done) ->
        counter = 1
        encoder = vcd.createVcdiffEncoder hashedDictionary: hashedDict
        inp = new RepeatableRead testData, 6
        out = new CountingWrite (nwrites) ->
          # One flush was triggered. Default window is 4096 so we should
          # have 1 write if it weren't for flush.
          nwrites.should.equal 6
          done()
        flush = new Flush ->
          encoder.flush()
        inp.pipe(flush).pipe(encoder).pipe(out)

  describe 'VcdiffDecoder', ->
    it 'should throw if no options provided', ->
      vcd.createVcdiffDecoder.should.throw Error, /Invalid dictionary/
      vcd.VcdiffDecoder.should.throw Error, /Invalid dictionary/
      vcd.vcdiffDecode.should.throw Error, /Invalid dictionary/

    it 'should throw if options does not contain dictionary', ->
      (-> vcd.createVcdiffDecoder allowVcdTarget: true)
        .should.throw Error, /Invalid dictionary/
      (-> vcd.VcdiffDecoder allowVcdTarget: true)
        .should.throw Error, /Invalid dictionary/
      (-> vcd.vcdiffDecode allowVcdTarget: true)
        .should.throw Error, /Invalid dictionary/

    it 'should throw if dictionary is not instance of Buffer', ->
      (-> vcd.createVcdiffDecoder dictionary: true)
        .should.throw Error, /Invalid dictionary/
      (-> vcd.VcdiffDecoder dictionary: true)
        .should.throw Error, /Invalid dictionary/
      (-> vcd.vcdiffDecode dictionary: true)
        .should.throw Error, /Invalid dictionary/

    it 'should be created if everything is OK', ->
      encoder = vcd.createVcdiffDecoder dictionary: new Buffer 'test'
      encoder.should.be.instanceof vcd.VcdiffDecoder

    it 'should respond to public interface methods', ->
      encoder = vcd.createVcdiffDecoder dictionary: new Buffer 'test'
      encoder.should.respondTo 'flush'
      encoder.should.respondTo 'close'

    it 'should accept flags', ->
      encoder = vcd.createVcdiffDecoder
        dictionary: new Buffer 'test'
        allowVcdTarget: true
      encoder.should.be.instanceof vcd.VcdiffDecoder

    it 'should return error if input invalid', (done) ->
      dict = new Buffer 'this is a test dictionary not very long'
      testData = 'this is a test dictionary not very long a test dictionary not'
      (-> vcd.vcdiffDecodeSync testData, dictionary: dict)
      .should.throw /Vcdiff decode error/

      vcd.vcdiffDecode testData, dictionary: dict, (err, data) ->
        err.message.should.contain.string 'Vcdiff decode error'
        done()

    xit 'should set flags correctly', ->
      # No idea how to test it yet. Perhaps, use spies.

  describe 'there and back again', ->
    dict = new Buffer 'this is a test dictionary not very long'
    hashedDict = new vcd.HashedDictionary dict
    testData = 'this is a test dictionary not very long a test dictionary not'

    it 'should encode and decode sync', ->
      e = vcd.vcdiffEncodeSync testData, hashedDictionary: hashedDict
      e.should.have.length.below testData.length
      e = vcd.vcdiffDecodeSync e, dictionary: dict
      e.toString().should.equal testData

    it 'should encode and decode async', (done) ->
      vcd.vcdiffEncode testData, hashedDictionary: hashedDict, (err, enc) ->
        enc.should.have.length.below testData.length
        vcd.vcdiffDecode enc, dictionary: dict, (err, dec) ->
          dec.toString().should.equal testData
          done()

    it 'should work with stream api', (done) ->
      encoder = vcd.createVcdiffEncoder hashedDictionary: hashedDict
      decoder = vcd.createVcdiffDecoder dictionary: dict

      class ReadingStuff extends stream.Readable
        constructor: (@data) ->
          super()

        _read: ->
          @push @data
          @data = null

      class WritingStuff extends stream.Writable
        constructor: (finishcb) ->
          super()
          @nread = 0
          @data_ = []
          @on 'finish', ->
            finishcb(Buffer.concat(@data_, @nread))

        _write: (chunk, encoding, next) ->
          chunk.should.be.instanceof Buffer
          @nread += chunk.length
          @data_.push(chunk)
          next()

      testIn = new ReadingStuff testData
      testOut = new WritingStuff (result) ->
        result.should.have.length.below testData.length
        encodedIn = new ReadingStuff(result)
        decodedOut = new WritingStuff (result) ->
          result.toString().should.equal testData
          done()
        encodedIn.pipe(decoder).pipe(decodedOut)
      testIn.pipe(encoder).pipe(testOut)

    it 'should not crash', (done) ->
      zlib = require 'zlib'
      encoder = vcd.createVcdiffEncoder hashedDictionary: hashedDict
      # encoder = zlib.createGzip()

      chunks = 0
      encoder.on 'data', (arg) ->
        if chunks == 2
          return encoder.end()
        encoder.write new Buffer 1024
        encoder.flush()
        chunks++
      encoder.on 'end', ->
        chunks.should.equal 2
        done()

      encoder.write new Buffer 1024
      encoder.flush()
