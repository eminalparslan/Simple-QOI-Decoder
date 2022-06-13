# QOI to PPM Image Converter
A small project to get more familiar with C and image encoding/decoding.

[QOI](https://qoiformat.org/) is a new image format that is almost as effective as PNG, but both simpler and faster.
[PPM](http://netpbm.sourceforge.net/doc/ppm.html) is an extremely simple image format that is literally just a sequence of RGB values.
The focus of this project was just to decode QOI, which is why I am converting it into such a simple image format.

This project was inspired by [this video](https://www.youtube.com/watch?v=EFUYNoFRHQI) from Reducible.

## Future Work:
Because of the simplicity of the PPM format, it can't handle transparency. In the future, I might change the output format from PPM to [PAM](http://netpbm.sourceforge.net/doc/pam.html),
which is from the same [Netbpm](http://netpbm.sourceforge.net/doc/index.html) suite and allows for transparent images through alpha channels.

## Helpful Resources:
 - [PPM Documentation](http://netpbm.sourceforge.net/doc/index.html#transparency)
 - [QOI Spec](https://qoiformat.org/qoi-specification.pdf) (yes, it's only one page)
 - Test images are from [here](https://qoiformat.org/)
