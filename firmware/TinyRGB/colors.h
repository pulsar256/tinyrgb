/*
 * HSV.h
 *
 * Created: 23.09.2013 17:38:22
 * Author: Paul Rogalinski, paul@paul.vc
 * HSV Conversion adapted from http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb
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
	unsigned char v;
} HsvColor;

/*
 * converts a HSV value to RGB using integer arithmetic only. 
 * Current implementation has some flaws, expect the converted values to have some jitter when cycling through H
 */
RgbColor hsvToRgb(HsvColor hsv)
{
	RgbColor rgb;
	unsigned char region, remainder, p, q, t;
	
	if(hsv.s == 0) 
	{
		rgb.r = rgb.g = rgb.b = hsv.v;
		return rgb;
	}

	region = hsv.h / 43;
	remainder = (hsv.h - (region * 43)) * 6;

	p = (hsv.v * (255 - hsv.s)) >> 8;
	q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
	t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;
	
	switch(region)
	{
		case 0:
		rgb.r = hsv.v; rgb.g = t; rgb.b = p; break;
		case 1:
		rgb.r = q; rgb.g = hsv.v; rgb.b = p; break;
		case 2:
		rgb.r = p; rgb.g = hsv.v; rgb.b = t; break;
		case 3:
		rgb.r = p; rgb.g = q; rgb.b = hsv.v; break;
		case 4:
		rgb.r = t; rgb.g = p; rgb.b = hsv.v; break;
		default:
		rgb.r = hsv.v; rgb.g = p; rgb.b = q; break;
	}
	return rgb;
}

#endif /* COLORS_H_ */