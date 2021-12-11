Spinner 2 - Spin2 compiler frontend
===================================

This is a C-based command-line frontend for the Parallax Spin2 compiler.

It's mostly a proof-of-concept at this point, but contributions are welcome.

## Usage

Simply run it on the command line with the top file of your choosing.
```
spinner2 Some_File.spin2
```
... will result in a `Some_File.binary` executable file that you can load with [loadp2](https://github.com/totalspectrum/loadp2).

Additionally, the following (useful) arguments are currently accepted:
| | |
|-|-|
|`--verbose`|Emit additional diagnostic messages|
|`--document`|Generate documentation files|
|`-o filename`|Change output filename (currently needs to include the extension, might change this later)|


## Building

### Prerequisites
 - an x86-based computer (or appropriate emulation thereof)
 - rake (any reasonably recent version)
   - on Windows, simply use [RubyInstaller](https://rubyinstaller.org)
   - on Linux, it may be included in the ruby package, it may be a seperate "ruby-rake" package or you may have to run `sudo gem install rake` (the sudo is neccessary to install the command-line tool)
 - gcc (any reasonably recent version) **WITH 32 BIT LIBRARIES**
   - On Windows, [MinGW](https://osdn.net/projects/mingw/) works (installing is a bit of a pain).
   - On Linux, simply install gcc from your package manager and, IDK, google how to install the 32 bit libs on your distro. Or maybe you already have them?
 - (Linux only) WINE. Install it from your package manager or whereever.

### Actually doing it

Simply run
```
rake
```
and the build should commence and emit a `spinner2(.exe)` file.
