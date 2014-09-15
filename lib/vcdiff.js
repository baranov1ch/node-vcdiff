/*!
 * node-vcdiff
 * https://github.com/baranov1ch/node-vcdiff
 *
 * Copyright 2014 Alexey Baranov <me@kotiki.cc>
 * Released under the MIT license
 */

var assert = require('assert').ok;
var binding = require('../build/Release/vcdiff')
var stream = require('stream');
var util = require('util');

exports.MAX_MAX_TARGET_FILE_SIZE = 1 << 32 - 1
exports.MIN_MAX_TARGET_FILE_SIZE = 1;
exports.DEFAULT_MAX_TARGET_FILE_SIZE = 1 << 26;  // 64Mb

exports.MAX_MAX_TARGET_WINDOW_SIZE = 1 << 32 - 1
exports.MIN_MAX_TARGET_WINDOW_SIZE = 1;
exports.DEFAULT_MAX_TARGET_WINDOW_SIZE = 1 << 26;  // 64Mb

// encoding fewer than 64 bytes per chunk is stupid.
exports.MIN_MIN_ENCODE_WINDOW_SIZE = 64;
exports.MAX_MIN_ENCODE_WINDOW_SIZE = Infinity;
exports.DEFAULT_MIN_ENCODE_WINDOW_SIZE = 4 * 1024;  // 4Kb


exports.codes = {
  VCD_INIT_ERROR : binding.INIT_ERROR,
  VCD_ENCODE_ERROR : binding.ENCODE_ERROR,
  VCD_DECODE_ERROR : binding.DECODE_ERROR,
};

Object.keys(exports.codes).forEach(function(k) {
  exports.codes[exports.codes[k]] = k;
});

exports.HashedDictionary = binding.HashedDictionary;
exports.VcdiffEncoder = VcdiffEncoder;
exports.VcdiffDecoder = VcdiffDecoder;

exports.createVcdiffEncoder = function(o) {
  return new VcdiffEncoder(o);
};

exports.createVcdiffDecoder = function(o) {
  return new VcdiffDecoder(o);
};

exports.vcdiffEncode = function(buffer, opts, callback) {
  return vcdiffBuffer(new VcdiffEncoder(opts), buffer, callback);
};

exports.vcdiffEncodeSync = function(buffer, opts) {
  return vcdiffBufferSync(new VcdiffEncoder(opts), buffer);
};

exports.vcdiffDecode = function(buffer, opts, callback) {
  return vcdiffBuffer(new VcdiffDecoder(opts), buffer, callback);
};

exports.vcdiffDecodeSync = function(buffer, opts) {
  return vcdiffBufferSync(new VcdiffDecoder(opts), buffer);
};

function vcdiffBuffer(engine, buffer, callback) {
  if (!(callback instanceof Function))
    throw new Error('callback should be a Function instance');
  var buffers = [];
  var nread = 0;

  engine.on('error', onError);
  engine.on('end', onEnd);

  engine.end(buffer);
  flow();

  function flow() {
    var chunk;
    while (null !== (chunk = engine.read())) {
      buffers.push(chunk);
      nread += chunk.length;
    }
    engine.once('readable', flow);
  }

  function onError(err) {
    engine.removeListener('end', onEnd);
    engine.removeListener('readable', flow);
    callback(err);
  }

  function onEnd() {
    var buf = Buffer.concat(buffers, nread);
    buffers = [];
    callback(null, buf);
    engine.close();
  }
};

function vcdiffBufferSync(engine, buffer) {
  if (typeof buffer === 'string')
    buffer = new Buffer(buffer);
  if (!Buffer.isBuffer(buffer))
    throw new TypeError('Not a string or buffer');

  return engine._processChunk(buffer, true);
};

function VcdiffEncoder(opts) {
  if (!(this instanceof VcdiffEncoder)) return new VcdiffEncoder(opts);
  Vcdiff.call(this, opts, binding.ENCODE);
};

function VcdiffDecoder(opts) {
  if (!(this instanceof VcdiffDecoder)) return new VcdiffDecoder(opts);
  Vcdiff.call(this, opts, binding.DECODE);
};

function Vcdiff(opts, mode) {
  opts = opts || {};
  this._minEncodeWindowSize =
    opts.minEncodeWindowSize || exports.DEFAULT_MIN_ENCODE_WINDOW_SIZE;

  stream.Transform.call(this, opts);

  if (mode === binding.ENCODE) {
    var flags = binding.VCD_STANDARD_FORMAT;
    var targetMatches = false;

    if (!(opts.hashedDictionary instanceof Object)) {
      throw new Error('Must provide HashedDictionary');
    }

    if (opts.interleaved === true)
      flags |= binding.VCD_FORMAT_INTERLEAVED;

    if (opts.checksum === true)
      flags |= binding.VCD_FORMAT_CHECKSUM;

    if (opts.json === true)
      flags |= binding.VCD_FORMAT_JSON;

    if (opts.targetMatches === true)
      targetMatches = true;

    if (opts.encodeWindowSize) {
      if (opts.encodeWindowSize < exports.MIN_MIN_ENCODE_WINDOW_SIZE ||
          opts.encodeWindowSize > exports.MAX_MIN_ENCODE_WINDOW_SIZE)
        throw new Error('Invalid encode window size: ' +
                        opts.encodeWindowSize);
    }

    this._handle = new binding.Vcdiff(
        mode, opts.hashedDictionary, targetMatches, flags);
  } else if (mode === binding.DECODE) {
    if (!Buffer.isBuffer(opts.dictionary))
      throw new Error('Invalid dictionary: it should be a Buffer instance');
    var allowVcd = true;
    var maxTargetFileSize = exports.DEFAULT_MAX_TARGET_FILE_SIZE;
    var maxTargetWindowSize = exports.DEFAULT_MAX_TARGET_WINDOW_SIZE;

    if (opts.allowVcdTarget === false)
      allowVcd = false;

    if (opts.maxTargetFileSize) {
      if (opts.maxTargetFileSize < exports.MIN_MAX_TARGET_FILE_SIZE ||
          opts.maxTargetFileSize > exports.MAX_MAX_TARGET_FILE_SIZE)
        throw new Error('Invalid max target file size: ' +
                        opts.maxTargetFileSize);
    }

    if (opts.maxTargetWindowSize) {
      if (opts.maxTargetWindowSize < exports.MIN_MAX_TARGET_WINDOW_SIZE ||
          opts.maxTargetWindowSize > exports.MAX_MAX_TARGET_WINDOW_SIZE)
        throw new Error('Invalid max target window size: ' +
                        opts.maxTargetWindowSize);
    }

    this._handle = new binding.Vcdiff(mode,
                                      opts.dictionary,
                                      allowVcd,
                                      maxTargetFileSize,
                                      maxTargetWindowSize);
  } else {
    throw new Error('invalid mode: neither ENCODE nor DECODE');
  }

  var self = this;
  this._mode = mode;
  this._hadError = false;
  this._handle.onerror = function(message, errno) {
    // there is no way to cleanly recover.
    // continuing only obscures problems.
    self._handle = null;
    self._hadError = true;

    var error = new Error(message);
    error.errno = errno;
    error.code = exports.codes[errno];
    self.emit('error', error);
  };

  this._closed = false;
  this._forceFlush = false;
  this._nread = 0;
  this._buffers = [];

  this.once('end', this.close);
};

util.inherits(Vcdiff, stream.Transform);

Vcdiff.prototype.close = function(callback) {
  if (callback)
    process.nextTick(callback);

  if (this._closed)
    return;

  this._closed = true;

  this._handle.close();

  var self = this;
  process.nextTick(function() {
    self.emit('close');
  });
};

Vcdiff.prototype.flush = function(callback) {
  var ws = this._writableState;
  if (ws.ended) {
    if (callback)
      process.nextTick(callback);
  } else if (ws.ending) {
    if (callback)
      this.once('end', callback);
  } else if (ws.needDrain) {
    var self = this;
    this.once('drain', function() {
      self.flush(callback);
    });
  } else {
    this._forceFlush = true;
    this.write(new Buffer(0), '', callback);
  }
};

Vcdiff.prototype._flush = function(callback) {
  this._forceFlush = true;
  this._transform(new Buffer(0), '', callback);
};

Vcdiff.prototype._transform = function(chunk, encoding, cb) {
  var ws = this._writableState;
  var ending = ws.ending || ws.ended;
  var last = ending && (!chunk || ws.length === chunk.length);

  if (chunk !== null && !Buffer.isBuffer(chunk))
    return cb(new Error('invalid input'));

  if (this._closed)
    return cb(new Error('vcdiff binding closed'));

  this._processChunk(chunk, last , cb);
};

Vcdiff.prototype._processChunk = function(chunk, isLast, cb) {
  var self = this;

  var async = cb instanceof Function;

  if (!async) {
    var error;
    this.on('error', function(er) {
      error = er;
    });

    assert(!this._closed, 'vcdiff binding closed');
    var res = this._handle.writeSync(isLast, chunk);

    if (this._hadError)
      throw error;

    assert(res[1] === true, 'sync should finish in one pass');

    this.close();

    return res[0];
  }

  this._nread += chunk.length;
  this._buffers.push(chunk);

  if (this._nread >= this._minEncodeWindowSize || this._forceFlush ||
      this._mode === binding.DECODE) {
    assert(!this._closed, 'vcdiff binding closed');
    chunk = Buffer.concat(this._buffers, this._nread);
    var req = this._handle.write(isLast, chunk);
    req.buffer = chunk;
    req.callback = callback;
  } else {
    cb();
  }

  function callback(out, finished) {
    if (self._hadError)
      return;

    self.push(out);

    self._nread = 0;
    self._buffers = [];
    self._forceFlush = false;

    cb();
  }
};

util.inherits(VcdiffEncoder, Vcdiff);
util.inherits(VcdiffDecoder, Vcdiff);
