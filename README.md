# NOYC

A simple noise maker application written in C to practice noise generation implementations, image formats and learn C itself.

Current implementation is bare minimum. Which is ton generate an `example.tif` file with `rand` generated value.

- `make build` to build
- `make run` to run 
- `make clean` to clean up

## Runtime parameters

`noyc <octaves_int> <persistence_double> <base_frequency_double> <base_amplitude_double>`

- `octaves` - number of octaves to loop through while generating the coordinate
- `persistence` - rate at which the amplitude decreases
- `base frequency` - initial octave frequency
- `base amplitude` - initial octave amplitude

## Notable examples

`noyc 8 0.55 0.005 1.5` - see `img/example_1.tif`

`noyc 8 0.75 0.00095 0.5` - see `img/example_2.tif`

`noyc 1 0.50 0.0125 1.` - see `img/example_3.tif`

