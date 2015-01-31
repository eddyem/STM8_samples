function u = U(ADU)
	adu = uint32(ADU)
	adu *= 3584
	adu = bitshift(adu, -17);
	u = uint32(10*adu);
endfunction
