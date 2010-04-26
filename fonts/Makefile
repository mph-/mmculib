FONTGEN = ../fontgen/fontgen


all: font7x5_2.def font14x10_2.def font28x20_2.def font7x5_1.def font14x10_1.def font28x20_1.def font7x6_1.def font14x12_1.def font28x24_1.def


font7x5_2.def: font7x5_2.txt
	$(FONTGEN) $(@:.def=) < $^  > $@

font14x10_2.def: font7x5_2.txt
	$(FONTGEN) $(@:.def=) -zoom 2 < $^  > $@

font28x20_2.def: font7x5_2.txt
	$(FONTGEN) $(@:.def=) -zoom 4 < $^  > $@

font7x5_1.def: font7x5_1.txt
	$(FONTGEN) $(@:.def=) < $^  > $@

font14x10_1.def: font7x5_1.txt
	$(FONTGEN) $(@:.def=) -zoom 2 < $^  > $@

font28x20_1.def: font7x5_1.txt
	$(FONTGEN) $(@:.def=) -zoom 4 < $^  > $@

font7x6_1.def: font7x6_1.txt
	$(FONTGEN) $(@:.def=) < $^  > $@

font14x12_1.def: font7x6_1.txt
	$(FONTGEN) $(@:.def=) -zoom 2 < $^  > $@

font28x24_1.def: font7x6_1.txt
	$(FONTGEN) $(@:.def=) -zoom 4 < $^  > $@
