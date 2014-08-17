all : ../tigr.c

.PHONY : ../tigr.c

tigr_font.h : font.png incbin.pl
	perl incbin.pl $@ $< tigr_font

../tigr.c : tigr_font.h bundle.pl
	perl bundle.pl $@ tigr_master.c

# We don't rebuild this by default.
upscale :
	fxc /nologo /Tvs_2_0 /Evs_main /VntigrUpscaleVSCode /O3 /Zpr /Gfa /Fhtigr_upscale_vs.h tigr_upscale.hlsl
	fxc /nologo /Tps_2_0 /Eps_main /VntigrUpscalePSCode /O3 /Zpr /Gfa /Fhtigr_upscale_ps.h tigr_upscale.hlsl
