/*
  GRIDqlc Premium RGB Script — Test Script
  "Color Wave Premium"

  Demonstrates that this .js file can be encrypted to .qlcscript
  and decrypted at runtime by LicenseManager when user is licensed.

  This script creates a color wave that moves across the fixture grid.
  Hue shifts over time creating a rainbow wave effect.

  Copyright (c) Filip Olszewski — GRIDqlc Premium Content
*/

var scriptVersion = 2;
var scriptAPIVersion = 2;

function rgbMapSetColors(width, height, rgb, step)
{
    var map = new Array(height);
    for (var y = 0; y < height; y++)
    {
        map[y] = new Array(width);
        for (var x = 0; x < width; x++)
        {
            // Wave offset: each column is phase-shifted
            var phase = (step + x * 20) % 360;
            var hue = phase / 360.0;

            // HSV to RGB conversion (s=1, v=1)
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
                default: r = 0; g = 0; b = 0;
            }

            var ri = Math.round(r * 255);
            var gi = Math.round(g * 255);
            var bi = Math.round(b * 255);

            map[y][x] = (ri << 16) | (gi << 8) | bi;
        }
    }
    return map;
}

function rgbMapStepCount(width, height)
{
    return 360;
}
