/*
 * random.h
 *
 * Created: 15.09.2013 03:02:10
 *  Author: pulsar
 */ 


#ifndef RANDOM_H_
#define RANDOM_H_g


inline uint8_t clamp(int value)
{
	return value < 0 ? 0 : (value > 0xff ? 0xff : value);
}

unsigned short getSeed()
{
	unsigned short seed = 0;
	unsigned short *p = (unsigned short*) (RAMEND+1);
	extern unsigned short __heap_start;
	while (p >= &__heap_start + 1) seed ^= * (--p);
	return seed;
}

// @see http://en.wikipedia.org/wiki/Random_number_generation
unsigned long m_w = 0;
unsigned long m_z = 0;
unsigned long getRandom()
{
	if (m_z == 0) m_z = getSeed();
	if (m_w == 0) m_w = getSeed();
	m_z = 36969L * (m_z & 65535L) + (m_z >> 16);
	m_w = 18000L * (m_w & 65535L) + (m_w >> 16);
	return (m_z << 16) + m_w;
}

uint8_t getNextRandom(uint8_t max,int offset, uint8_t current)
{
	int tmp;
	do
	{
		tmp   = clamp(((getRandom() % max) + offset));
	}
	while (tmp ==  current);
	return tmp;
}


#endif /* RANDOM_H_ */