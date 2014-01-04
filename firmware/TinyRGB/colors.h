/*
 * HSV.h
 *
 * Created: 23.09.2013 17:38:22
 * Author: Paul Rogalinski, paul@paul.vc
 */


#ifndef COLORS_H_
#define COLORS_H_

typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
} RgbColor;

typedef struct
{
	unsigned char h;
	unsigned char s;
	unsigned char l;
} HslColor;

/*
 * adapted from http://tog.acm.org/resources/GraphicsGems/gems/HSLtoRGB.c
 * converts a HSL value to an RGB value.
 */
void hslToRgb(HslColor* hsl, RgbColor* rgb)
{
	int v;

	v = (hsl->l < 128) 
		? (long int)(hsl->l * (256 + hsl->s)) >> 8 
		: (((long int)(hsl->l + hsl->s) << 8) - (long int)hsl->l * hsl->s) >> 8;

	if (v <= 0) 
	{
		rgb->r = rgb->g = rgb->b = 0;
	} 
	else 
	{
		long int m;
		long int sextant;
		long int fract, vsf, mid1, mid2;
		int hue = hsl->h;

		m = hsl->l + hsl->l - v;
		hue *= 6;
		sextant = hue >> 8;
		fract = hue - (sextant << 8);
		vsf = v * fract * (v - m) / v >> 8;
		mid1 = m + vsf;
		mid2 = v - vsf;
		switch (sextant) {
			case 0: rgb->r = v;    rgb->g = mid1;  rgb->b = m;    break;
			case 1: rgb->r = mid2; rgb->g = v;     rgb->b = m;    break;
			case 2: rgb->r = m;    rgb->g = v;     rgb->b = mid1; break;
			case 3: rgb->r = m;    rgb->g = mid2;  rgb->b = v;    break;
			case 4: rgb->r = mid1; rgb->g = m;     rgb->b = v;    break;
			case 5: rgb->r = v;    rgb->g = m;     rgb->b = mid2; break;
		}
	}
}

#endif /* COLORS_H_ */