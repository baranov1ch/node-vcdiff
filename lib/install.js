var exec = require('child_process').exec;
var logSymbols = require('log-symbols');
var path = require('path');

var vcDiffDir = path.resolve(__dirname, '../src/third-party/open-vcdiff');

var cmds = [
  'autoreconf -fiv',
  './configure --disable-shared --prefix="' + vcDiffDir + '"',
  'make && make install'
];
console.log(vcDiffDir);
exec(cmds.join(' && '), { cwd: vcDiffDir }, function(err) {
  if (err) {
    console.log(logSymbols.error, err);
    return;
  }

  console.log(logSymbols.success, ' open-vcdiff built successfully');
});
