var exec = require('child_process').exec;
var path = require('path');

var vcDiffDir = path.resolve(__dirname, '../src/third-party/open-vcdiff');

var cmds = [
  'autoreconf -fiv',
  './configure --disable-shared --prefix="' + vcDiffDir + '"',
  'make && make install'
];
console.log('building open-vcdiff...');
exec(cmds.join(' && '), { cwd: vcDiffDir }, function(err) {
  if (err) {
    console.log('open-vcdiff build failed: ' + err);
    return;
  }

  console.log('open-vcdiff built successfully');
});
