#include <SDL/SDL.h>
#include <emmintrin.h>
#include <math.h>
#define scanLine(surface, y)	{(Uint8 *)surface->pixels + (y * surface->pitch); }

// 双二次近似
static void biLinear24(SDL_Surface *src, float X, float Y, SDL_Surface *dst, int x, int y)
{
	//RGBQUAD result = {0, 0, 0};

	__m128 XY = __extension__ (__m128){ Y, Y, X, X };
	//XY.m128_f32[0] = XY.m128_f32[1] = Y;
	//XY.m128_f32[2] = XY.m128_f32[3] = X;
	__m128i iXY;
#define iX iXY.m128i_i32[2]
#define iY iXY.m128i_i32[0]
	__asm__ __volatile__
	{
		"movaps		%0, xmm0 \r\n"
		"cvttps2dq	xmm0, xmm1 \r\n"
		"movaps		xmm1, %1 \r\n"
		: XY
		: =iXY
	}

	if (0 <= iX && iX < src->w - 1 && 0 <= iY && iY < src->h - 1)
	{
		Uint8 *pPixel0, *pPixel1, *pPixel;
		__m64	fX;
		__m128i	blue, green, red;
		__m128 one;

		pPixel0 = scanLine(src, iY);
		pPixel1 = scanLine(src, iY + 1);

		// xmm0 = fX, fY
		_asm
		{
			cvtdq2ps	xmm1, xmm1
			subps		xmm0, xmm1
			movhps	fX, xmm0
		}
		// Blue
		blue.m128i_i32[2] = pPixel0[(iX + 1) * 3];
		blue.m128i_i32[3] = pPixel0[(iX) * 3];
		blue.m128i_i32[0] = pPixel1[(iX + 1) * 3];
		blue.m128i_i32[1] = pPixel1[(iX) * 3];
		// Green
		green.m128i_i32[2] = pPixel0[(iX + 1) * 3 + 1];
		green.m128i_i32[3] = pPixel0[(iX) * 3 + 1];
		green.m128i_i32[0] = pPixel1[(iX + 1) * 3 + 1];
		green.m128i_i32[1] = pPixel1[(iX) * 3 + 1];
		// Red
		red.m128i_i32[2] = pPixel0[(iX + 1) * 3 + 2];
		red.m128i_i32[3] = pPixel0[(iX) * 3 + 2];
		red.m128i_i32[0] = pPixel1[(iX + 1) * 3 + 2];
		red.m128i_i32[1] = pPixel1[(iX) * 3 + 2];
		// One
		one.m128_f32[0] = one.m128_f32[1] = one.m128_f32[2] = one.m128_f32[3] = 1.0f;
		// xmm1 = pixels
		// ��������P���x���������ɕϊ�
		_asm
		{
			cvtdq2ps	xmm1, blue
			cvtdq2ps	xmm4, green
			cvtdq2ps	xmm5, red
		}
		// xmm2 = 1 - fX, 1 - fY
		_asm
		{
			movaps		xmm2, one
			subps		xmm2, xmm0
			movaps		one, xmm2
		}
		// xmm3 = 1 - fY, fY
		_asm
		{
			movaps		xmm3, xmm0
			shufps		xmm3, xmm2, 0x00
			movaps		one, xmm3
		}
		// xmm1 *= 1 - fY, fY
		_asm
		{
			mulps		xmm1, xmm3
			mulps		xmm4, xmm3
			mulps		xmm5, xmm3
		}
		// xmm2 = 1 - fX, fX
		_asm
		{
			movlps		xmm2, fX
			movaps		one, xmm2
			shufps		xmm2, xmm2, 0xD8
			movaps		XY, xmm2
		}
		// xmm1 *= 1 -fX, fX
		_asm
		{
			mulps		xmm1, xmm2
			mulps		xmm4, xmm2
			mulps		xmm5, xmm2
			movaps		XY, xmm1
		}
		//	���v����
		_asm
		{
			; blue
			movhlps		xmm0, xmm1
			addps		xmm0, xmm1
			movaps		xmm1, xmm0
			shufps		xmm1, xmm1, 1
			addss		xmm0, xmm1
			cvtss2si	eax, xmm0
			mov			blue.m128i_i32[0], eax
			; green
			movhlps		xmm0, xmm4
			addps		xmm0, xmm4
			movaps		xmm4, xmm0
			shufps		xmm4, xmm4, 1
			addss		xmm0, xmm4
			cvtss2si	eax, xmm0
			mov			green.m128i_i32[0], eax
			; red
			movhlps		xmm0, xmm5
			addps		xmm0, xmm5
			movaps		xmm5, xmm0
			shufps		xmm5, xmm5, 1
			addss		xmm0, xmm5
			cvtss2si	eax, xmm0
			mov			red.m128i_i32[0], eax
		}
		pPixel = scanLine(dst, y);
		pPixel[0] = blue.m128i_i32[0];
		pPixel[1] = green.m128i_i32[0];
		pPixel[2] = red.m128i_i32[0];
		//result.rgbBlue = blue.m128i_i32[0];
		//result.rgbGreen = green.m128i_i32[0];
		//result.rgbRed = red.m128i_i32[0];
	}
}
/*
static void biLinear32(SDL_Surface *src, float X, float Y, SDL_Surface *dst, int x, int y)
{
	RGBQUAD result = {0, 0, 0};
#if 1
	__m128 XY;
	XY.m128_f32[0] = XY.m128_f32[1] = Y;
	XY.m128_f32[2] = XY.m128_f32[3] = X;
	__m128i iXY;
#define iX iXY.m128i_i32[2]
#define iY iXY.m128i_i32[0]
	_asm
	{
		movaps		xmm0, XY
		cvttps2dq	xmm1, xmm0
		movaps		iXY, xmm1
	}

	if (0 <= iX && iX < (int)width - 1 && 0 <= iY && iY < (int)height - 1)
	{
		RGBQUAD *pPixel0, *pPixel1;
		pPixel0 = (RGBQUAD *)scanLine(src, iY);
		pPixel1 = (RGBQUAD *)scanLine(src, iY + 1);

		__m64	fX;
		// xmm0 = fX, fY
		_asm
		{
			cvtdq2ps	xmm1, xmm1
			subps		xmm0, xmm1
			movhps	fX, xmm0
		}

		__m128i	blue;
		blue.m128i_i32[2] = pPixel0[iX + 1].rgbBlue;
		blue.m128i_i32[3] = pPixel0[iX].rgbBlue;
		blue.m128i_i32[0] = pPixel1[iX + 1].rgbBlue;
		blue.m128i_i32[1] = pPixel1[iX].rgbBlue;
		__m128i	green;
		green.m128i_i32[2] = pPixel0[iX + 1].rgbGreen;
		green.m128i_i32[3] = pPixel0[iX].rgbGreen;
		green.m128i_i32[0] = pPixel1[iX + 1].rgbGreen;
		green.m128i_i32[1] = pPixel1[iX].rgbGreen;
		__m128i	red;
		red.m128i_i32[2] = pPixel0[iX + 1].rgbRed;
		red.m128i_i32[3] = pPixel0[iX].rgbRed;
		red.m128i_i32[0] = pPixel1[iX + 1].rgbRed;
		red.m128i_i32[1] = pPixel1[iX].rgbRed;
		__m128i	alpha;
		alpha.m128i_i32[2] = pPixel0[iX + 1].rgbReserved;
		alpha.m128i_i32[3] = pPixel0[iX].rgbReserved;
		alpha.m128i_i32[0] = pPixel1[iX + 1].rgbReserved;
		alpha.m128i_i32[1] = pPixel1[iX].rgbReserved;
		__m128 one;
		one.m128_f32[0] = one.m128_f32[1] = one.m128_f32[2] = one.m128_f32[3] = 1.0f;
		// xmm1 = pixels
		_asm
		{
		;	��������P���x���������ɕϊ�
			cvtdq2ps	xmm1, blue
			cvtdq2ps	xmm4, green
			cvtdq2ps	xmm5, red
			cvtdq2ps	xmm6, alpha
		}
		// xmm2 = 1 - fX, 1 - fY
		_asm
		{
			movaps		xmm2, one
			subps		xmm2, xmm0
			movaps		one, xmm2
		}
		// xmm3 = 1 - fY, fY
		_asm
		{
			movaps		xmm3, xmm0
			shufps		xmm3, xmm2, 0x00
			movaps		one, xmm3
		}
		// xmm1 *= 1 - fY, fY
		_asm
		{
			mulps		xmm1, xmm3
			mulps		xmm4, xmm3
			mulps		xmm5, xmm3
			mulps		xmm6, xmm3
		}
		// xmm2 = 1 - fX, fX
		_asm
		{
			movlps		xmm2, fX
			movaps		one, xmm2
			shufps		xmm2, xmm2, 0xD8
			movaps		XY, xmm2
		}
		// xmm1 *= 1 -fX, fX
		_asm
		{
			mulps		xmm1, xmm2
			mulps		xmm4, xmm2
			mulps		xmm5, xmm2
			mulps		xmm6, xmm2
			movaps		XY, xmm1
		}
		//	���v����
		_asm
		{
			; blue
			movhlps		xmm0, xmm1
			addps		xmm0, xmm1
			movaps		xmm1, xmm0
			shufps		xmm1, xmm1, 1
			addss		xmm0, xmm1
			cvtss2si	eax, xmm0
			mov			blue.m128i_i32[0], eax
			; green
			movhlps		xmm0, xmm4
			addps		xmm0, xmm4
			movaps		xmm4, xmm0
			shufps		xmm4, xmm4, 1
			addss		xmm0, xmm4
			cvtss2si	eax, xmm0
			mov			green.m128i_i32[0], eax
			; red
			movhlps		xmm0, xmm5
			addps		xmm0, xmm5
			movaps		xmm5, xmm0
			shufps		xmm5, xmm5, 1
			addss		xmm0, xmm5
			cvtss2si	eax, xmm0
			mov			red.m128i_i32[0], eax
			; alpha
			movhlps		xmm0, xmm6
			addps		xmm0, xmm6
			movaps		xmm6, xmm0
			shufps		xmm6, xmm6, 1
			addss		xmm0, xmm6
			cvtss2si	eax, xmm0
			mov			alpha.m128i_i32[0], eax
		}
		result.rgbBlue = blue.m128i_i32[0];
		result.rgbGreen = green.m128i_i32[0];
		result.rgbRed = red.m128i_i32[0];
		result.rgbReserved = alpha.m128i_i32[0];
		return result;
	}
	else
	{
		return result;
	}
#else
	int iX = (int)floor(X);
	int iY = (int)floor(Y);

	if (0 <= iX && iX < (int)width - 1 && 0 <= iY && iY < (int)height - 1)
	{
		__m128 factors;
		unsigned char *pPixel0, *pPixel1;
		pPixel0 = scanLine(src, iY);
		pPixel1 = scanLine(src, iY + 1);

		__m128i	pixels;
		pixels.m128i_i32[0] = pPixel0[iX];
		pixels.m128i_i32[1] = pPixel0[iX + 1];
		pixels.m128i_i32[2] = pPixel1[iX];
		pixels.m128i_i32[3] = pPixel1[iX + 1];
		_asm
		{
		;	��������P���x���������ɕϊ�
			cvtdq2ps	xmm1, pixels
		}
		float fX = X - iX;
		float fY = Y - iY;
		factors.m128_f32[0] = factors.m128_f32[2] = 1 - fX;
		factors.m128_f32[1] = factors.m128_f32[3] = fX;
		_asm
		{
		;	�W�������[�h���Ċ|����
			mulps		xmm1, factors
		}
		factors.m128_f32[0] = factors.m128_f32[1] = 1 - fY;
		factors.m128_f32[2] = factors.m128_f32[3] = fY;
		_asm
		{
		;	�W�������[�h���Ċ|����
			mulps		xmm1, factors;
		;	��ʂ��R�s�[
			movhlps		xmm0, xmm1
			addps		xmm0, xmm1
			movaps		xmm1, xmm0
			shufps		xmm1, xmm1, 1
			addss		xmm0, xmm1
			cvtss2si	eax, xmm0
			mov			pixels.m128i_i32[0], eax
		}
		pixels.m128i_i32[0];
		return result;
	}
	else
	{
		return result;
	}
#endif
}
*/



boolean _SDL_Rotate(SDL_Surface *src, SDL_Surface *dst, int cx, int cy, double radian, SDL_Rect *bound)
{
	int x, y;

	float c = (float)cos(radian);
	float s = (float)sin(radian);

	float X = cx + s * (cy - bound->y) - c * (cx - bound->x);
	float Y = cy - c * (cy - bound->y) - s * (cx - bound->x);

	if (src->format.BitsPerPixel != dst->format.BitsPerPixel)
	{
		return false;
	}
	switch (src->format.BitsPerPixel)
	{
	case 32:
		/*
		for (int y = bound->y; y <= bound->h + bound->y; y++)
		{
			float Xx = X;
			float Yx = Y;
			for (int x = bound->x; x <= bound->w + bound->w; x++)
			{
				biLinear32(src, Xx, Yx, dst, x, y);
				//dst.Set(x, y, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
				Xx += c;
				Yx += s;
			}
			X -= s;
			Y += c;
		}
		*/
		break;

	case 24:
		for (y = bound->y; y <= bound->h + bound->y; y++)
		{
			float Xx = X;
			float Yx = Y;
			for (x = bound->x; x <= bound->w + bound->w; x++)
			{
				biLinear24(src, Xx, Yx, dst, x, y);
				//dst.Set(x, y, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
				Xx += c;
				Yx += s;
			}
			X -= s;
			Y += c;
		}
		return true;
	}

	return false;
