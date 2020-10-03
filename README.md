# libphm

PHM: Simple Portable Half float(fp16) Map loader/saver in C99.

`libphm` is for reading and storing fp16 image in simplest manner.

## File format

File format is based on PFM(Portable Float Map).

http://www.pauldebevec.com/Research/HDR/PFM/

We adapt Oyranos' proposal.

https://www.oyranos.org/2015/03/portable-float-map-with-16-bit-half/index.html

Use `Ph` for grayscale fp16, `PH` for RGB fp16.
No support for 2 or 4+ channels.

## Endianness

Little and Big endian should be supported.

## Example

* simple: Simple PHM example.
* examples/phm2exr: PHM to OpenEXR convert example(uses TinyEXR https://github.com/syoyo/tinyexr).

## Other fp16 format(s)

OpenEXR(EXR) is a well known fp16 file format.
You can read it with TinyEXR https://github.com/syoyo/tinyexr in C++ and pytinyexr https://pypi.org/project/pytinyexr/ or `imageio` package in Python

## TODO

* [ ] Support BFLOAT16 format(define new tag name such like `PBHF`?).
* [ ] UTF-8 filepath and wchar support on Windows.

## License

Unlicense(public domain) or MIT license(copyright 2020 Syoyo Fujita)
