/*
 *  Copyright (C) 2002-2021  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include <string.h>
#include <math.h>
#include "dosbox.h"
#include "inout.h"
#include "vga.h"
#include "mem.h"
#include "pic.h"
#include "render.h"
#include "mapper.h"

static void write_crtc_index_other(Bitu /*port*/,Bitu val,Bitu /*iolen*/) {
	vga.other.index=(Bit8u)val;
}

static Bitu read_crtc_index_other(Bitu /*port*/,Bitu /*iolen*/) {
	return vga.other.index;
}

static void write_crtc_data_other(Bitu /*port*/,Bitu val,Bitu /*iolen*/) {
	switch (vga.other.index) {
	case 0x00:		//Horizontal total
		if (vga.other.htotal ^ val) VGA_StartResize();
		vga.other.htotal=(Bit8u)val;
		break;
	case 0x01:		//Horizontal displayed chars
		if (vga.other.hdend ^ val) VGA_StartResize();
		vga.other.hdend=(Bit8u)val;
		break;
	case 0x02:		//Horizontal sync position
		vga.other.hsyncp=(Bit8u)val;
		break;
	case 0x03:		//Horizontal sync width
		if (machine==MCH_TANDY) vga.other.vsyncw=(Bit8u)(val >> 4);
		else vga.other.vsyncw = 16; // The MC6845 has a fixed v-sync width of 16 lines
		vga.other.hsyncw=(Bit8u)(val & 0xf);
		break;
	case 0x04:		//Vertical total
		if (vga.other.vtotal ^ val) VGA_StartResize();
		vga.other.vtotal=(Bit8u)val;
		break;
	case 0x05:		//Vertical display adjust
		if (vga.other.vadjust ^ val) VGA_StartResize();
		vga.other.vadjust=(Bit8u)val;
		break;
	case 0x06:		//Vertical rows
		if (vga.other.vdend ^ val) VGA_StartResize();
		vga.other.vdend=(Bit8u)val;
		break;
	case 0x07:		//Vertical sync position
		vga.other.vsyncp=(Bit8u)val;
		break;
	case 0x09:		//Max scanline
		val &= 0x1f; // VGADOC says bit 0-3 but the MC6845 datasheet says bit 0-4
 		if (vga.other.max_scanline ^ val) VGA_StartResize();
		vga.other.max_scanline=(Bit8u)val;
		break;
	case 0x0A:	/* Cursor Start Register */
		vga.other.cursor_start = (Bit8u)(val & 0x3f);
		vga.draw.cursor.sline = (Bit8u)(val&0x1f);
		vga.draw.cursor.enabled = ((val & 0x60) != 0x20);
		break;
	case 0x0B:	/* Cursor End Register */
		vga.other.cursor_end = (Bit8u)(val&0x1f);
		vga.draw.cursor.eline = (Bit8u)(val&0x1f);
		break;
	case 0x0C:	/* Start Address High Register */
		// Bit 12 (depending on video mode) and 13 are actually masked too,
		// but so far no need to implement it.
		vga.config.display_start=(vga.config.display_start & 0x00FF) | ((val&0x3F) << 8);
		break;
	case 0x0D:	/* Start Address Low Register */
		vga.config.display_start=(vga.config.display_start & 0xFF00) | val;
		break;
	case 0x0E:	/*Cursor Location High Register */
		vga.config.cursor_start&=0x00ff;
		vga.config.cursor_start|=((Bit8u)val) << 8;
		break;
	case 0x0F:	/* Cursor Location Low Register */
		vga.config.cursor_start&=0xff00;
		vga.config.cursor_start|=(Bit8u)val;
		break;
	case 0x10:	/* Light Pen High */
		vga.other.lightpen &= 0xff;
		vga.other.lightpen |= (val & 0x3f)<<8;		// only 6 bits
		break;
	case 0x11:	/* Light Pen Low */
		vga.other.lightpen &= 0xff00;
		vga.other.lightpen |= (Bit8u)val;
		break;
	default:
		LOG(LOG_VGAMISC,LOG_NORMAL)("MC6845:Write %" sBitfs(X) " to illegal index %x",val,vga.other.index);
	}
}
static Bitu read_crtc_data_other(Bitu /*port*/,Bitu /*iolen*/) {
	switch (vga.other.index) {
	case 0x00:		//Horizontal total
		return vga.other.htotal;
	case 0x01:		//Horizontal displayed chars
		return vga.other.hdend;
	case 0x02:		//Horizontal sync position
		return vga.other.hsyncp;
	case 0x03:		//Horizontal and vertical sync width
		if (machine==MCH_TANDY)
			return vga.other.hsyncw | (vga.other.vsyncw << 4);
		else return vga.other.hsyncw;
	case 0x04:		//Vertical total
		return vga.other.vtotal;
	case 0x05:		//Vertical display adjust
		return vga.other.vadjust;
	case 0x06:		//Vertical rows
		return vga.other.vdend;
	case 0x07:		//Vertical sync position
		return vga.other.vsyncp;
	case 0x09:		//Max scanline
		return vga.other.max_scanline;
	case 0x0A:	/* Cursor Start Register */
		return vga.other.cursor_start;
	case 0x0B:	/* Cursor End Register */
		return vga.other.cursor_end;
	case 0x0C:	/* Start Address High Register */
		return (Bit8u)(vga.config.display_start >> 8);
	case 0x0D:	/* Start Address Low Register */
		return (Bit8u)(vga.config.display_start & 0xff);
	case 0x0E:	/*Cursor Location High Register */
		return (Bit8u)(vga.config.cursor_start >> 8);
	case 0x0F:	/* Cursor Location Low Register */
		return (Bit8u)(vga.config.cursor_start & 0xff);
	case 0x10:	/* Light Pen High */
		return (Bit8u)(vga.other.lightpen >> 8);
	case 0x11:	/* Light Pen Low */
		return (Bit8u)(vga.other.lightpen & 0xff);
	default:
		LOG(LOG_VGAMISC,LOG_NORMAL)("MC6845:Read from illegal index %x",vga.other.index);
	}
	return (Bitu)(~0);
}

static void write_lightpen(Bitu port,Bitu /*val*/,Bitu) {
	switch (port) {
	case 0x3db:	// Clear lightpen latch
		vga.other.lightpen_triggered = false;
		break;
	case 0x3dc:	// Preset lightpen latch
		if (!vga.other.lightpen_triggered) {
			vga.other.lightpen_triggered = true; // TODO: this shows at port 3ba/3da bit 1

			float timeInFrame = PIC_FullIndex()-vga.draw.delay.framestart;
			float timeInLine = fmodf(timeInFrame,vga.draw.delay.htotal);
			Bitu current_scanline = (Bitu)(timeInFrame / vga.draw.delay.htotal);

			vga.other.lightpen = (Bit16u)((vga.draw.address_add/2) * (current_scanline/2));
			vga.other.lightpen += (Bit16u)((timeInLine / vga.draw.delay.hdend) *
				((float)(vga.draw.address_add/2)));
		}
		break;
	}
}

static float hue_offset = 0.0;
static Bit8u cga_comp = 0;
static bool new_cga = 0;

static Bit8u cga16_val = 0;
static void update_cga16_color(void);
static Bit8u herc_pal = 0;

static void cga16_color_select(Bit8u val) {
	cga16_val = val;
	update_cga16_color();
}

static void update_cga16_color(void) {
// New algorithm based on code by reenigne
// Works in all CGA graphics modes/color settings and can simulate older and newer CGA revisions
	static const float tau = 6.28318531; // == 2*pi
	static const float ns = 567.0/440;  // degrees of hue shift per nanosecond

	float tv_brightness = 0.0; // hardcoded for simpler implementation
	float tv_saturation = (new_cga ? 0.7 : 0.6);

	bool bw = (vga.tandy.mode_control&4) != 0;
	bool color_sel = (cga16_val&0x20) != 0;
	bool background_i = (cga16_val&0x10) != 0;	// Really foreground intensity, but this is what the CGA schematic calls it.
	bool bpp1 = (vga.tandy.mode_control&0x10) != 0;
	Bit8u overscan = cga16_val&0x0f;  // aka foreground colour in 1bpp mode

	float chroma_coefficient = new_cga ? 0.29 : 0.72;
	float b_coefficient = new_cga ? 0.07 : 0;
	float g_coefficient = new_cga ? 0.22 : 0;
	float r_coefficient = new_cga ? 0.1 : 0;
	float i_coefficient = new_cga ? 0.32 : 0.28;
	float rgbi_coefficients[0x10];
	for (int c = 0; c < 0x10; c++) {
		float v = 0;
		if ((c & 1) != 0)
			v += b_coefficient;
		if ((c & 2) != 0)
			v += g_coefficient;
		if ((c & 4) != 0)
			v += r_coefficient;
		if ((c & 8) != 0)
			v += i_coefficient;
		rgbi_coefficients[c] = v;
	}

	// The pixel clock delay calculation is not accurate for 2bpp, but the difference is small and a more accurate calculation would be too slow.
	static const float rgbi_pixel_delay = 15.5*ns;
	static const float chroma_pixel_delays[8] = {
		0,        // Black:   no chroma
		35*ns,    // Blue:    no XORs
		44.5*ns,  // Green:   XOR on rising and falling edges
		39.5*ns,  // Cyan:    XOR on falling but not rising edge
		44.5*ns,  // Red:     XOR on rising and falling edges
		39.5*ns,  // Magenta: XOR on falling but not rising edge
		44.5*ns,  // Yellow:  XOR on rising and falling edges
		39.5*ns}; // White:   XOR on falling but not rising edge
	float pixel_clock_delay;
	int o = overscan == 0 ? 15 : overscan;
	if (overscan == 8)
		pixel_clock_delay = rgbi_pixel_delay;
	else {
		float d = rgbi_coefficients[o];
		pixel_clock_delay = (chroma_pixel_delays[o & 7]*chroma_coefficient + rgbi_pixel_delay*d)/(chroma_coefficient + d);
	}
	pixel_clock_delay -= 21.5*ns;  // correct for delay of color burst

	float hue_adjust = (-(90-33)-hue_offset+pixel_clock_delay)*tau/360.0;
	float chroma_signals[8][4];
	for (Bit8u i=0; i<4; i++) {
		chroma_signals[0][i] = 0;
		chroma_signals[7][i] = 1;
		for (Bit8u j=0; j<6; j++) {
			static const float phases[6] = {
				270 - 21.5*ns,  // blue
				135 - 29.5*ns,  // green
				180 - 21.5*ns,  // cyan
				  0 - 21.5*ns,  // red
				315 - 29.5*ns,  // magenta
				 90 - 21.5*ns}; // yellow/burst
			// All the duty cycle fractions are the same, just under 0.5 as the rising edge is delayed 2ns more than the falling edge.
			static const float duty = 0.5 - 2*ns/360.0;

			// We have a rectangle wave with period 1 (in units of the reciprocal of the color burst frequency) and duty
			// cycle fraction "duty" and phase "phase". We band-limit this wave to frequency 2 and sample it at intervals of 1/4.
			// We model our band-limited wave with 4 frequency components:
			//   f(x) = a + b*sin(x*tau) + c*cos(x*tau) + d*sin(x*2*tau)
			// Then:
			//   a =   integral(0, 1, f(x)*dx) = duty
			//   b = 2*integral(0, 1, f(x)*sin(x*tau)*dx) = 2*integral(0, duty, sin(x*tau)*dx) = 2*(1-cos(x*tau))/tau
			//   c = 2*integral(0, 1, f(x)*cos(x*tau)*dx) = 2*integral(0, duty, cos(x*tau)*dx) = 2*sin(duty*tau)/tau
			//   d = 2*integral(0, 1, f(x)*sin(x*2*tau)*dx) = 2*integral(0, duty, sin(x*4*pi)*dx) = 2*(1-cos(2*tau*duty))/(2*tau)
			float a = duty;
			float b = 2.0*(1.0-cosf(duty*tau))/tau;
			float c = 2.0*sinf(duty*tau)/tau;
			float d = 2.0*(1.0-cosf(duty*2*tau))/(2*tau);

			float x = (phases[j] + 21.5*ns + pixel_clock_delay)/360.0 + i/4.0;

			chroma_signals[j+1][i] = a + b*sinf(x*tau) + c*cosf(x*tau) + d*sinf(x*2*tau);
		}
	}
	Bitu CGApal[4] = {
		overscan,
		static_cast<Bitu>(2 + (color_sel||bw ? 1 : 0) + (background_i ? 8 : 0)),
		static_cast<Bitu>(4 + (color_sel&&!bw? 1 : 0) + (background_i ? 8 : 0)),
		static_cast<Bitu>(6 + (color_sel||bw ? 1 : 0) + (background_i ? 8 : 0))
	};
	for (Bit8u x=0; x<4; x++) {	 // Position of pixel in question
		bool even = (x & 1) == 0;
		for (Bit8u bits=0; bits<(even ? 0x10 : 0x40); ++bits) {
			float Y=0, I=0, Q=0;
			for (Bit8u p=0; p<4; p++) {  // Position within color carrier cycle
				// generate pixel pattern.
				Bit8u rgbi;
				if (bpp1)
					rgbi = ((bits >> (3-p)) & (even ? 1 : 2)) != 0 ? overscan : 0;
				else
					if (even)
						rgbi = CGApal[(bits >> (2-(p&2)))&3];
					else
						rgbi = CGApal[(bits >> (4-((p+1)&6)))&3];
				Bit8u c = rgbi & 7;
				if (bw && c != 0)
					c = 7;

				// calculate composite output
				float chroma = chroma_signals[c][(p+x)&3]*chroma_coefficient;
				float composite = chroma + rgbi_coefficients[rgbi];

				Y+=composite;
				if (!bw) { // burst on
					I+=composite*2*cosf(hue_adjust + (p+x)*tau/4.0);
					Q+=composite*2*sinf(hue_adjust + (p+x)*tau/4.0);
				}
			}

			float contrast = 1 - tv_brightness;

			Y = (contrast*Y/4.0) + tv_brightness; if (Y>1.0) Y=1.0; if (Y<0.0) Y=0.0;
			I = (contrast*I/4.0) * tv_saturation; if (I>0.5957) I=0.5957; if (I<-0.5957) I=-0.5957;
			Q = (contrast*Q/4.0) * tv_saturation; if (Q>0.5226) Q=0.5226; if (Q<-0.5226) Q=-0.5226;

			static const float gamma = 2.2;

			float R = Y + 0.9563*I + 0.6210*Q;	R = (R - 0.075) / (1-0.075); if (R<0) R=0; if (R>1) R=1;
			float G = Y - 0.2721*I - 0.6474*Q;	G = (G - 0.075) / (1-0.075); if (G<0) G=0; if (G>1) G=1;
			float B = Y - 1.1069*I + 1.7046*Q;	B = (B - 0.075) / (1-0.075); if (B<0) B=0; if (B>1) B=1;
			R = powf(R, gamma);
			G = powf(G, gamma);
			B = powf(B, gamma);

			int r = static_cast<int>(255*powf( 1.5073*R -0.3725*G -0.0832*B, 1/gamma)); if (r<0) r=0; if (r>255) r=255;
			int g = static_cast<int>(255*powf(-0.0275*R +0.9350*G +0.0670*B, 1/gamma)); if (g<0) g=0; if (g>255) g=255;
			int b = static_cast<int>(255*powf(-0.0272*R -0.0401*G +1.1677*B, 1/gamma)); if (b<0) b=0; if (b>255) b=255;

			Bit8u index = bits | ((x & 1) == 0 ? 0x30 : 0x80) | ((x & 2) == 0 ? 0x40 : 0);
			RENDER_SetPal(index,r,g,b);
		}
	}
}

static void IncreaseHue(bool pressed) {
	if (!pressed)
		return;
	hue_offset += 5.0;
	update_cga16_color();
	LOG_MSG("Hue at %f",hue_offset); 
}

static void DecreaseHue(bool pressed) {
	if (!pressed)
		return;
	hue_offset -= 5.0;
	update_cga16_color();
	LOG_MSG("Hue at %f",hue_offset); 
}

static void write_cga_color_select(Bitu val) {
	vga.tandy.color_select=val;
	switch(vga.mode) {
	case  M_TANDY4: {
		Bit8u base = (val & 0x10) ? 0x08 : 0;
		Bit8u bg = val & 0xf;
		if (vga.tandy.mode_control & 0x4)	// cyan red white
			VGA_SetCGA4Table(bg, 3+base, 4+base, 7+base);
		else if (val & 0x20)				// cyan magenta white
			VGA_SetCGA4Table(bg, 3+base, 5+base, 7+base);
		else								// green red brown
			VGA_SetCGA4Table(bg, 2+base, 4+base, 6+base);
		vga.tandy.border_color = bg;
		vga.attr.overscan_color = bg;
		break;
	}
	case M_TANDY2:
		VGA_SetCGA2Table(0,val & 0xf);
		vga.attr.overscan_color = 0;
		break;
	case M_CGA16:
		cga16_color_select(val);
		break;
	case M_TEXT:
		vga.tandy.border_color = val & 0xf;
		vga.attr.overscan_color = 0;
		break;
	default: //Else unhandled values warning
		break;
	}
}

static void write_cga(Bitu port,Bitu val,Bitu /*iolen*/) {
	switch (port) {
	case 0x3d8:
		vga.tandy.mode_control=(Bit8u)val;
		vga.attr.disabled = (val&0x8)? 0: 1;
		if (vga.tandy.mode_control & 0x2) {		// graphics mode
			if (vga.tandy.mode_control & 0x10) {// highres mode
				if (cga_comp==1 || (cga_comp==0 && !(val&0x4))) {	// composite display
					VGA_SetMode(M_CGA16);		// composite ntsc 640x200 16 color mode
				} else {
					VGA_SetMode(M_TANDY2);
				}
			} else {							// lowres mode
				if (cga_comp==1) {				// composite display
					VGA_SetMode(M_CGA16);		// composite ntsc 640x200 16 color mode
				} else {
					VGA_SetMode(M_TANDY4);
				}
			}

			write_cga_color_select(vga.tandy.color_select);
		} else {
			VGA_SetMode(M_TANDY_TEXT);
		}
		VGA_SetBlinking(val & 0x20);
		break;
	case 0x3d9: // color select
		write_cga_color_select(val);
		break;
	}
}

static void CGAModel(bool pressed) {
	if (!pressed) return;
	new_cga = !new_cga;
	update_cga16_color();
	LOG_MSG("%s model CGA selected", new_cga ? "Late" : "Early");
}

static void Composite(bool pressed) {
	if (!pressed) return;
	if (++cga_comp>2) cga_comp=0;
	LOG_MSG("Composite output: %s",(cga_comp==0)?"auto":((cga_comp==1)?"on":"off"));
	// switch RGB and Composite if in graphics mode
	if (vga.tandy.mode_control & 0x2)
		write_cga(0x3d8,vga.tandy.mode_control,1);
}

static void tandy_update_palette() {
	// TODO mask off bits if needed
	if (machine == MCH_TANDY) {
		switch (vga.mode) {
		case M_TANDY2:
			VGA_SetCGA2Table(vga.attr.palette[0],
				vga.attr.palette[vga.tandy.color_select&0xf]);
			break;
		case M_TANDY4:
			if (vga.tandy.gfx_control & 0x8) {
				// 4-color high resolution - might be an idea to introduce M_TANDY4H
				VGA_SetCGA4Table( // function sets both medium and highres 4color tables
					vga.attr.palette[0], vga.attr.palette[1],
					vga.attr.palette[2], vga.attr.palette[3]);
			} else {
				Bit8u color_set = 0;
				Bit8u r_mask = 0xf;
				if (vga.tandy.color_select & 0x10) color_set |= 8; // intensity
				if (vga.tandy.color_select & 0x20) color_set |= 1; // Cyan Mag. White
				if (vga.tandy.mode_control & 0x04) {			// Cyan Red White
					color_set |= 1;
					r_mask &= ~1;
				}
				VGA_SetCGA4Table(
					vga.attr.palette[vga.tandy.color_select&0xf],
					vga.attr.palette[(2|color_set)& vga.tandy.palette_mask],
					vga.attr.palette[(4|(color_set& r_mask))& vga.tandy.palette_mask],
					vga.attr.palette[(6|color_set)& vga.tandy.palette_mask]);
			}
			break;
		default:
			break;
		}
	} else {
		// PCJr
		switch (vga.mode) {
		case M_TANDY2:
			VGA_SetCGA2Table(vga.attr.palette[0],vga.attr.palette[1]);
			break;
		case M_TANDY4:
			VGA_SetCGA4Table(
				vga.attr.palette[0], vga.attr.palette[1],
				vga.attr.palette[2], vga.attr.palette[3]);
			break;
		default:
			break;
		}
	}
}

void VGA_SetModeNow(VGAModes mode);

static void TANDY_FindMode(void) {
	if (vga.tandy.mode_control & 0x2) {
		if (vga.tandy.gfx_control & 0x10) {
			if (vga.mode==M_TANDY4) {
				VGA_SetModeNow(M_TANDY16);
			} else VGA_SetMode(M_TANDY16);
		}
		else if (vga.tandy.gfx_control & 0x08) {
			VGA_SetMode(M_TANDY4);
		}
		else if (vga.tandy.mode_control & 0x10)
			VGA_SetMode(M_TANDY2);
		else {
			if (vga.mode==M_TANDY16) {
				VGA_SetModeNow(M_TANDY4);
			} else VGA_SetMode(M_TANDY4);
		}
		tandy_update_palette();
	} else {
		VGA_SetMode(M_TANDY_TEXT);
	}
}

static void PCJr_FindMode(void) {
	if (vga.tandy.mode_control & 0x2) {
		if (vga.tandy.mode_control & 0x10) {
			/* bit4 of mode control 1 signals 16 colour graphics mode */
			if (vga.mode==M_TANDY4) VGA_SetModeNow(M_TANDY16); // TODO lowres mode only
			else VGA_SetMode(M_TANDY16);
		} else if (vga.tandy.gfx_control & 0x08) {
			/* bit3 of mode control 2 signals 2 colour graphics mode */
			VGA_SetMode(M_TANDY2);
		} else {
			/* otherwise some 4-colour graphics mode */
			if (vga.mode==M_TANDY16) VGA_SetModeNow(M_TANDY4);
			else VGA_SetMode(M_TANDY4);
		}
		tandy_update_palette();
	} else {
		VGA_SetMode(M_TANDY_TEXT);
	}
}

static void TandyCheckLineMask(void ) {
	if ( vga.tandy.extended_ram & 1 ) {
		vga.tandy.line_mask = 0;
	} else if ( vga.tandy.mode_control & 0x2) {
		vga.tandy.line_mask |= 1;
	}
	if ( vga.tandy.line_mask ) {
		vga.tandy.line_shift = 13;
		vga.tandy.addr_mask = (1 << 13) - 1;
	} else {
		vga.tandy.addr_mask = (Bitu)(~0);
		vga.tandy.line_shift = 0;
	}
}

static void write_tandy_reg(Bit8u val) {
	switch (vga.tandy.reg_index) {
	case 0x0:
		if (machine==MCH_PCJR) {
			vga.tandy.mode_control=val;
			VGA_SetBlinking(val & 0x20);
			PCJr_FindMode();
			if (val&0x8) vga.attr.disabled &= ~1;
			else vga.attr.disabled |= 1;
		} else {
			LOG(LOG_VGAMISC,LOG_NORMAL)("Unhandled Write %2X to tandy reg %X",val,vga.tandy.reg_index);
		}
		break;
	case 0x1:	/* Palette mask */
		vga.tandy.palette_mask = val;
		tandy_update_palette();
		break;
	case 0x2:	/* Border color */
		vga.tandy.border_color=val;
		break;
	case 0x3:	/* More control */
		vga.tandy.gfx_control=val;
		if (machine==MCH_TANDY) TANDY_FindMode();
		else PCJr_FindMode();
		break;
	case 0x5:	/* Extended ram page register */
		// Bit 0 enables extended ram
		// Bit 7 Switches clock, 0 -> cga 28.6 , 1 -> mono 32.5
		vga.tandy.extended_ram = val;
		//This is a bit of a hack to enable mapping video memory differently for highres mode
		TandyCheckLineMask();
		VGA_SetupHandlers();
		break;
	default:
		if ((vga.tandy.reg_index & 0xf0) == 0x10) { // color palette
			vga.attr.palette[vga.tandy.reg_index-0x10] = val&0xf;
			tandy_update_palette();
		} else
			LOG(LOG_VGAMISC,LOG_NORMAL)("Unhandled Write %2X to tandy reg %X",val,vga.tandy.reg_index);
	}
}

static void write_tandy(Bitu port,Bitu val,Bitu /*iolen*/) {
	switch (port) {
	case 0x3d8:
		val &= 0x3f; // only bits 0-6 are used
		if (vga.tandy.mode_control ^ val) {
			vga.tandy.mode_control=(Bit8u)val;
			if (val&0x8) vga.attr.disabled &= ~1;
			else vga.attr.disabled |= 1;
			TandyCheckLineMask();
			VGA_SetBlinking(val & 0x20);
			TANDY_FindMode();
			VGA_StartResize();
		}
		break;
	case 0x3d9:
		vga.tandy.color_select=val;
		tandy_update_palette();
		break;
	case 0x3da:
		vga.tandy.reg_index=(Bit8u)val;
		//if (val&0x10) vga.attr.disabled |= 2;
		//else vga.attr.disabled &= ~2;
		break;
//	case 0x3dd:	//Extended ram page address register:
//		break;
	case 0x3de:
		write_tandy_reg((Bit8u)val);
		break;
	case 0x3df:
		// CRT/processor page register
		// See the comments on the PCJr version of this register.
		// A difference to it is:
		// Bit 3-5: Processor page CPU_PG
		// The remapped range is 32kB instead of 16. Therefore CPU_PG bit 0
		// appears to be ORed with CPU A14 (to preserve some sort of
		// backwards compatibility?), resulting in odd pages being mapped
		// as 2x16kB. Implemeted in vga_memory.cpp Tandy handler.

		vga.tandy.line_mask = (Bit8u)(val >> 6);
		vga.tandy.draw_bank = val & ((vga.tandy.line_mask&2) ? 0x6 : 0x7);
		vga.tandy.mem_bank = (val >> 3) & 7;
		TandyCheckLineMask();
		VGA_SetupHandlers();
		break;
	}
}

static void write_pcjr(Bitu port,Bitu val,Bitu /*iolen*/) {
	switch (port) {
	case 0x3da:
		if (vga.tandy.pcjr_flipflop) write_tandy_reg((Bit8u)val);
		else {
			vga.tandy.reg_index=(Bit8u)val;
			if (vga.tandy.reg_index & 0x10)
				vga.attr.disabled |= 2;
			else vga.attr.disabled &= ~2;
		}
		vga.tandy.pcjr_flipflop=!vga.tandy.pcjr_flipflop;
		break;
	case 0x3df:
		// CRT/processor page register

		// Bit 0-2: CRT page PG0-2
		// In one- and two bank modes, bit 0-2 select the 16kB memory
		// area of system RAM that is displayed on the screen.
		// In 4-banked modes, bit 1-2 select the 32kB memory area.
		// Bit 2 only has effect when the PCJR upgrade to 128k is installed.

		// Bit 3-5: Processor page CPU_PG
		// Selects the 16kB area of system RAM that is mapped to
		// the B8000h IBM PC video memory window. Since A14-A16 of the
		// processor are unconditionally replaced with these bits when
		// B8000h is accessed, the 16kB area is mapped to the 32kB
		// range twice in a row. (Scuba Venture writes across the boundary)

		// Bit 6-7: Video Address mode
		// 0: CRTC addresses A0-12 directly, accessing 8k characters
		//    (+8k attributes). Used in text modes (one bank).
		//    PG0-2 in effect. 16k range.
		// 1: CRTC A12 is replaced with CRTC RA0 (see max_scanline).
		//    This results in the even/odd scanline two bank system.
		//    PG0-2 in effect. 16k range.
		// 2: Documented as unused. CRTC addresses A0-12, PG0 is replaced
		//    with RA1. Looks like nonsense.
		//    PG1-2 in effect. 32k range which cannot be used completely.
		// 3: CRTC A12 is replaced with CRTC RA0, PG0 is replaced with
		//    CRTC RA1. This results in the 4-bank mode.
		//    PG1-2 in effect. 32k range.

		vga.tandy.line_mask = (Bit8u)(val >> 6);
		vga.tandy.draw_bank = val & ((vga.tandy.line_mask&2) ? 0x6 : 0x7);
		vga.tandy.mem_bank = (val >> 3) & 7;
		vga.tandy.draw_base = &MemBase[vga.tandy.draw_bank * 16 * 1024];
		vga.tandy.mem_base = &MemBase[vga.tandy.mem_bank * 16 * 1024];
		TandyCheckLineMask();
		VGA_SetupHandlers();
		break;
	}
}

static void CycleHercPal(bool pressed) {
	if (!pressed) return;
	if (++herc_pal>2) herc_pal=0;
	Herc_Palette();
	VGA_DAC_CombineColor(1,7);
}

void Herc_Palette(void) {
	switch (herc_pal) {
	case 0:	// White
		VGA_DAC_SetEntry(0x7,0x2a,0x2a,0x2a);
		VGA_DAC_SetEntry(0xf,0x3f,0x3f,0x3f);
		break;
	case 1:	// Amber
		VGA_DAC_SetEntry(0x7,0x34,0x20,0x00);
		VGA_DAC_SetEntry(0xf,0x3f,0x34,0x00);
		break;
	case 2:	// Green
		VGA_DAC_SetEntry(0x7,0x00,0x26,0x00);
		VGA_DAC_SetEntry(0xf,0x00,0x3f,0x00);
		break;
	}
}

static void write_hercules(Bitu port,Bitu val,Bitu /*iolen*/) {
	switch (port) {
	case 0x3b8: {
		// the protected bits can always be cleared but only be set if the
		// protection bits are set
		if (vga.herc.mode_control&0x2) {
			// already set
			if (!(val&0x2)) {
				vga.herc.mode_control &= ~0x2;
				VGA_SetMode(M_HERC_TEXT);
			}
		} else {
			// not set, can only set if protection bit is set
			if ((val & 0x2) && (vga.herc.enable_bits & 0x1)) {
				vga.herc.mode_control |= 0x2;
				VGA_SetMode(M_HERC_GFX);
			}
		}
		if (vga.herc.mode_control&0x80) {
			if (!(val&0x80)) {
				vga.herc.mode_control &= ~0x80;
				vga.tandy.draw_base = &vga.mem.linear[0];
			}
		} else {
			if ((val & 0x80) && (vga.herc.enable_bits & 0x2)) {
				vga.herc.mode_control |= 0x80;
				vga.tandy.draw_base = &vga.mem.linear[32*1024];
			}
		}
		vga.draw.blinking = (val&0x20)!=0;
		vga.herc.mode_control &= 0x82;
		vga.herc.mode_control |= val & ~0x82;
		break;
		}
	case 0x3bf:
		if ( vga.herc.enable_bits ^ val) {
			vga.herc.enable_bits=val;
			// Bit 1 enables the upper 32k of video memory,
			// so update the handlers
			VGA_SetupHandlers();
		}
		break;
	}
}

/* static Bitu read_hercules(Bitu port,Bitu iolen) {
	LOG_MSG("read from Herc port %x",port);
	return 0;
} */

Bitu read_herc_status(Bitu /*port*/,Bitu /*iolen*/) {
	// 3BAh (R):  Status Register
	// bit   0  Horizontal sync
	//       1  Light pen status (only some cards)
	//       3  Video signal
	//     4-6	000: Hercules
	//			001: Hercules Plus
	//			101: Hercules InColor
	//			111: Unknown clone
	//       7  Vertical sync inverted

	float timeInFrame = PIC_FullIndex()-vga.draw.delay.framestart;
	Bit8u retval=0x72; // Hercules ident; from a working card (Winbond W86855AF)
					// Another known working card has 0x76 ("KeysoGood", full-length)
	if (timeInFrame < vga.draw.delay.vrstart ||
		timeInFrame > vga.draw.delay.vrend) retval |= 0x80;

	float timeInLine=fmodf(timeInFrame,vga.draw.delay.htotal);
	if (timeInLine >= vga.draw.delay.hrstart &&
		timeInLine <= vga.draw.delay.hrend) retval |= 0x1;

	// 688 Attack sub checks bit 3 - as a workaround have the bit enabled
	// if no sync active (corresponds to a completely white screen)
	if ((retval&0x81)==0x80) retval |= 0x8;
	return retval;
}


void VGA_SetupOther(void) {
	Bitu i;
	memset( &vga.tandy, 0, sizeof( vga.tandy ));
	vga.attr.disabled = 0;
	vga.config.bytes_skip=0;

	//Initialize values common for most machines, can be overwritten
	vga.tandy.draw_base = vga.mem.linear;
	vga.tandy.mem_base = vga.mem.linear;
	vga.tandy.addr_mask = 8*1024 - 1;
	vga.tandy.line_mask = 3;
	vga.tandy.line_shift = 13;

	if (machine==MCH_CGA || IS_TANDY_ARCH) {
		extern Bit8u int10_font_08[256 * 8];
		for (i=0;i<256;i++)	memcpy(&vga.draw.font[i*32],&int10_font_08[i*8],8);
		vga.draw.font_tables[0]=vga.draw.font_tables[1]=vga.draw.font;
	}
	if (machine==MCH_CGA || IS_TANDY_ARCH || machine==MCH_HERC) {
		IO_RegisterWriteHandler(0x3db,write_lightpen,IO_MB);
		IO_RegisterWriteHandler(0x3dc,write_lightpen,IO_MB);
	}
	if (machine==MCH_HERC) {
		extern Bit8u int10_font_14[256 * 14];
		for (i=0;i<256;i++)	memcpy(&vga.draw.font[i*32],&int10_font_14[i*14],14);
		vga.draw.font_tables[0]=vga.draw.font_tables[1]=vga.draw.font;
		MAPPER_AddHandler(CycleHercPal,MK_f11,0,"hercpal","Herc Pal");
	}
	if (machine==MCH_CGA) {
		IO_RegisterWriteHandler(0x3d8,write_cga,IO_MB);
		IO_RegisterWriteHandler(0x3d9,write_cga,IO_MB);
		MAPPER_AddHandler(IncreaseHue,MK_f11,MMOD2,"inchue","Inc Hue");
		MAPPER_AddHandler(DecreaseHue,MK_f11,0,"dechue","Dec Hue");
		MAPPER_AddHandler(CGAModel,MK_f11,MMOD1|MMOD2,"cgamodel","CGA Model");
		MAPPER_AddHandler(Composite,MK_f12,0,"cgacomp","CGA Comp");
	}
	if (machine==MCH_TANDY) {
		write_tandy( 0x3df, 0x0, 0 );
		IO_RegisterWriteHandler(0x3d8,write_tandy,IO_MB);
		IO_RegisterWriteHandler(0x3d9,write_tandy,IO_MB);
		IO_RegisterWriteHandler(0x3da,write_tandy,IO_MB);
		IO_RegisterWriteHandler(0x3de,write_tandy,IO_MB);
		IO_RegisterWriteHandler(0x3df,write_tandy,IO_MB);
	}
	if (machine==MCH_PCJR) {
		//write_pcjr will setup base address
		write_pcjr( 0x3df, 0x7 | (0x7 << 3), 0 );
		IO_RegisterWriteHandler(0x3da,write_pcjr,IO_MB);
		IO_RegisterWriteHandler(0x3df,write_pcjr,IO_MB);
	}
	if (machine==MCH_HERC) {
		Bitu base=0x3b0;
		for (Bitu i = 0; i < 4; i++) {
			// The registers are repeated as the address is not decoded properly;
			// The official ports are 3b4, 3b5
			IO_RegisterWriteHandler(base+i*2,write_crtc_index_other,IO_MB);
			IO_RegisterWriteHandler(base+i*2+1,write_crtc_data_other,IO_MB);
			IO_RegisterReadHandler(base+i*2,read_crtc_index_other,IO_MB);
			IO_RegisterReadHandler(base+i*2+1,read_crtc_data_other,IO_MB);
		}
		vga.herc.enable_bits=0;
		vga.herc.mode_control=0xa; // first mode written will be text mode
		vga.crtc.underline_location = 13;
		IO_RegisterWriteHandler(0x3b8,write_hercules,IO_MB);
		IO_RegisterWriteHandler(0x3bf,write_hercules,IO_MB);
		IO_RegisterReadHandler(0x3ba,read_herc_status,IO_MB);
	} else if (!IS_EGAVGA_ARCH) {
		Bitu base=0x3d0;
		for (Bitu port_ct=0; port_ct<4; port_ct++) {
			IO_RegisterWriteHandler(base+port_ct*2,write_crtc_index_other,IO_MB);
			IO_RegisterWriteHandler(base+port_ct*2+1,write_crtc_data_other,IO_MB);
			IO_RegisterReadHandler(base+port_ct*2,read_crtc_index_other,IO_MB);
			IO_RegisterReadHandler(base+port_ct*2+1,read_crtc_data_other,IO_MB);
		}
	}

}
