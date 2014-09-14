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

    it 'should throw if dictionary is not instance of Buffer', ->
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

    it 'should encode json for ascii', ->
      dict = new Buffer 'testmehatekillomgdieyoulittledumb', 'ascii'
      testData = new Buffer(
        'testmehatekillomgdieyoulittledumbdieyoulittledumb'
        'ascii')
      e = vcd.vcdiffEncodeSync(
        testData
        hashedDictionary: new vcd.HashedDictionary dict
        json: true)

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

  describe 'there and back again', ->
    dict = new Buffer 'testmehatekillomgdieyoulittledumb'
    hashedDict = new vcd.HashedDictionary dict
    testData = 'testmehatekillomgdieyoulittledumbdieyoulittledumb'

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

        _read: () ->
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
