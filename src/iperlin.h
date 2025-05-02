#ifndef IPERLIN_H_
#define IPERLIN_H_

/*
** Improved Perlin noise function
**
** Created by Ken Perlin and presented in SIGGRAPH 2002 paper
*/

double iperlin_at(double x, double y, double z);
double octave_iperlin_at(double x, double y, double z, int octaves, double persistence, double bfreq, double bam);

#endif // IPERLIN_H_
