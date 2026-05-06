/*
  GRIDqlc Premium RGB Script — Test Script

  Copyright (c) Filip Olszewski — GRIDqlc Premium Content
*/

var testAlgo;

(function() {

  var algo = new Object;
  algo.apiVersion = 2;
  algo.name = "Color Wave Premium";
  algo.author = "GRIDqlc";
  algo.acceptColors = 0;

  algo.rgbMap = function(width, height, rgb, step) {
    var map = new Array(height);
    for (var y = 0; y < height; y++) {
      map[y] = new Array(width);
      for (var x = 0; x < width; x++) {
        var phase = (step + x * 20) % 360;
        var hue = phase / 360.0;
        var h6 = hue * 6.0;
        var i = Math.floor(h6);
        var f = h6 - i;
        var q = 1.0 - f;
        var r, g, b;
        switch (i % 6) {
          case 0: r = 1;   g = f;   b = 0;   break;
          case 1: r = q;   g = 1;   b = 0;   break;
          case 2: r = 0;   g = 1;   b = f;   break;
          case 3: r = 0;   g = q;   b = 1;   break;
          case 4: r = f;   g = 0;   b = 1;   break;
          case 5: r = 1;   g = 0;   b = q;   break;
          default: r = 0;  g = 0;   b = 0;
        }
        map[y][x] = (Math.round(r * 255) << 16) |
                    (Math.round(g * 255) << 8)  |
                     Math.round(b * 255);
      }
    }
    return map;
  };

  algo.rgbMapStepCount = function(width, height) {
    return 360;
  };

  // Export
  testAlgo = algo;

})();
