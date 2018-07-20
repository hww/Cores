#include "stdafx.h"

extern clsPIC pic1;

clsSystem::clsSystem() {
	int nn;
	for (nn = 0; nn < sizeof(memory); nn+=4) {
		memory[nn>>2] = 0;
	}
	Reset();
};
void clsSystem::Reset()
{
	int nn;
	m_z = 88888888;
	m_w = 12345678;
	for (nn = 0; nn < 4096; nn++) {
		VideoMem[nn] = random();
		VideoMemDirty[nn] = true;
	}
	write_error = false;
	runstop = false;
	cpu1.system1 = this;
	refscreen = true;
};
	unsigned int clsSystem::Read(unsigned int ad, int sr) {
		int rr;
		unsigned __int8 sc;
		unsigned __int8 st;
		if (sr) {
			if (radr1 == 0)
				radr1 = ad;
			else if (radr2 == 0)
				radr2 = ad;
			else {
				if (random()&1)
					radr2 = ad;
				else
					radr1 = ad;
			}
		}
		if (ad < 134217728) {
			return memory[ad >> 2];
		}
		else if (((ad & 0xFFF00000) >> 20) == 0xFF4) {
			return (scratchpad[(ad & 32767) >> 2]);
		}
		else if ((ad & 0xfffc0000)==0xfffc0000) {
			return (rom[(ad >> 2) & 0x7fff]);
		}
		else if ((ad & 0xFFFF0000)==0xFFD00000) {
			return VideoMem[(ad>>2)& 0xFFF];
		}
		else if (keybd.IsSelected(ad)) {
			switch(ad & 0x1) {
			case 0:
				sc = keybd.Get();
				rr = ((int)sc<<24)|((int)sc << 16)|((int)sc<<8)|sc;
				break;
			case 1:
				st = keybd.GetStatus();
				rr = ((int)st<<24)|((int)st<<16)|((int)st<<8)|st;
				keybd_status = st;
				break;
			}
			return rr;
		}
		else if (pic1.IsSelected(ad)) {
			return pic1.Read(ad);
		}
		return 0;
	};
	int clsSystem::Write(unsigned int ad, unsigned int dat, unsigned int mask, int cr) {
		int nn;
		int ret;

		if (cr && (ad!=radr1 && ad!=radr2)) {
			ret = false;
			goto j1;
		}
		if (cr) {
			if (ad==radr1)
				radr1 = 0x00000000;
			if (ad==radr2)
				radr2 = 0x00000000;
		}
		if (ad < 134217728) {
			if (ad >= 0x10000 && ad < 0x20000) {
				write_error = true;
				ret = true;
				goto j1;
			}
			if ((ad & 0xfffffff0)==0x00c431a0) {
				ret = true;
			}
			switch(mask) {
			case 0xF:
				memory[ad>>2] = dat;
				break;
			case 0x1:
				memory[ad >> 2] &= 0xFFFFFF00;
				memory[ad >> 2] |= dat & 0xFF;
				break;
			case 0x2:
				memory[ad >> 2] &= 0xFFFF00FF;
				memory[ad >> 2] |= (dat & 0xFF) << 8;
				break;
			case 0x4:
				memory[ad >> 2] &= 0xFF00FFFF;
				memory[ad >> 2] |= (dat & 0xFF) << 16;
				break;
			case 0x8:
				memory[ad >> 2] &= 0x00FFFFFF;
				memory[ad >> 2] |= (dat & 0xFF) << 24;
				break;
			case 0x3:
				memory[ad >> 2] &= 0xFFFF0000;
				memory[ad >> 2] |= dat & 0xFFFF;
				break;
			case 0x6:
				memory[ad >> 2] &= 0xFF0000FF;
				memory[ad >> 2] |= (dat & 0xFFFF) << 8;
				break;
			case 0xC:
				memory[ad >> 2] &= 0x0000FFFF;
				memory[ad >> 2] |= (dat & 0xFFFF) << 16;
				break;
			}
		}
		else if (((ad & 0xFFF00000) >> 20) == 0xFF4) {
			unsigned int ad1 = (ad & 32767) >> 2;
			switch(mask) {
			case 0xF:
				scratchpad[ad1] = dat;
				break;
			case 0x1:
				scratchpad[ad1] &= 0xFFFFFF00;
				scratchpad[ad1] |= dat & 0xFF;
				break;
			case 0x2:
				scratchpad[ad1] &= 0xFFFF00FF;
				scratchpad[ad1] |= (dat & 0xFF) << 8;
				break;
			case 0x4:
				scratchpad[ad1] &= 0xFF00FFFF;
				scratchpad[ad1] |= (dat & 0xFF) << 16;
				break;
			case 0x8:
				scratchpad[ad1] &= 0x00FFFFFF;
				scratchpad[ad1] |= (dat & 0xFF) << 24;
				break;
			case 0x3:
				scratchpad[ad1] &= 0xFFFF0000;
				scratchpad[ad1] |= dat & 0xFFFF;
				break;
			case 0x6:
				scratchpad[ad1] &= 0xFF0000FF;
				scratchpad[ad1] |= (dat & 0xFFFF) << 8;
				break;
			case 0xC:
				scratchpad[ad1] &= 0x0000FFFF;
				scratchpad[ad1] |= (dat & 0xFFFF) << 16;
				break;
			}
		}
		else if ((ad & 0xFFFFFF00)==0xFFDC0600) {
			leds = dat;
		}
		else if ((ad & 0xfffc0000)==0xfffc0000) {
			rom[(ad>>2) & 0x7fff] = dat;
		}
		else if ((ad & 0xFFFF0000)==0xFFD00000) {
			VideoMem[(ad>>2)& 0xFFF] = dat;
			VideoMemDirty[(ad>>2)&0xfff] = true;
			refscreen = true;
		}
		else if (keybd.IsSelected(ad)) {
			switch(ad & 1) {
			case 1:	keybd_status = 0; pic1.irqKeyboard = keybd.GetStatus()==0x80; break;
			}
		}
		else if (pic1.IsSelected(ad)) {
			pic1.Write(ad,dat,0x3);
		}
		ret = true;
j1:
		for (nn = 0; nn < numDataBreakpoints; nn++) {
			if (ad==dataBreakpoints[nn]) {
				runstop = true;
			}
		}
		return ret;
	};
 	int clsSystem::random() {
		m_z = 36969 * (m_z & 65535) + (m_z >> 16);
		m_w = 18000 * (m_w & 65535) + (m_w >> 16);
		return (m_z << 16) + m_w;
	};
	unsigned __int64 clsSystem::ReadInsn(unsigned __int64 pc) {
		unsigned __int64 ir;
		unsigned __int64 ix1, ix2;
		ix1 = Read((unsigned int)(pc >> 2));
		ix2 = Read((unsigned int)((pc + 16LL) >> 2));
		switch (pc & 15LL) {
		case 0:	ir = ix1 + ((ix2 & 15) << 32LL); break;
		case 1:	ir = (ix1 >> 2) + ((ix2 & 63) << 30LL); break;
		case 2:	ir = (ix1 >> 4) + ((ix2 & 255) << 28LL); break;
		case 3:	ir = (ix1 >> 6) + ((ix2 & 1023) << 26LL); break;
		case 4: ir = (ix1 >> 8) + ((ix2 & 4095) << 24LL); break;
		case 5: ir = (ix1 >> 10) + ((ix2 & 16383) << 22LL); break;
		case 6: ir = (ix1 >> 12) + ((ix2 & 65535) << 20LL); break;
		case 7: ir = (ix1 >> 14) + ((ix2 & 0x3FFFF) << 18LL); break;
		case 8: ir = (ix1 >> 16) + ((ix2 & 0xFFFFF) << 16LL); break;
		case 9: ir = (ix1 >> 18) + ((ix2 & 0x3FFFFF) << 14LL); break;
		case 10: ir = (ix1 >> 20) + ((ix2 & 0xFFFFFF) << 12LL); break;
		case 11: ir = (ix1 >> 22) + ((ix2 & 0x3FFFFFF) << 10LL); break;
		case 12: ir = (ix1 >> 24) + ((ix2 & 0xFFFFFFF) << 8LL); break;
		case 13: ir = (ix1 >> 26) + ((ix2 & 0x3FFFFFFF) << 6LL); break;
		case 14: ir = (ix1 >> 28) + ((ix2 & 0xFFFFFFFFLL) << 4LL); break;
		case 15: ir = (ix1 >> 30) + ((ix2 & 0x3FFFFFFFFLL) << 2LL); break;
		}
		return (ir);
	}

