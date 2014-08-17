all : tigr.c

.PHONY : tigr.c

src/tigr_font.h : src/font.png src/incbin.pl
	perl src/incbin.pl $@ $< tigr_font

tigr.c : src/tigr_font.h src/bundle.pl
	perl src/bundle.pl $@ src/tigr_master.c

# We don't rebuild this by default.
upscale :
	fxc /nologo /Tvs_2_0 /Evs_main /VntigrUpscaleVSCode /O3 /Zpr /Gfa /Fhsrc/tigr_upscale_vs.h src/tigr_upscale.hlsl
	fxc /nologo /Tps_2_0 /Eps_main /VntigrUpscalePSCode /O3 /Zpr /Gfa /Fhsrc/tigr_upscale_ps.h src/tigr_upscale.hlsl
