function u = Ux(ADU, K)
	adu = uint32(ADU);
	adu *= K;
	adu = bitshift(adu, -17);
	u = uint32(adu);
endfunction
