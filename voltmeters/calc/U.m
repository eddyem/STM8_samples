function u = U(ADU)
	adu = uint32(ADU)
	adu *= 35840;
	adu = bitshift(adu, -17);
	u = uint32(adu);
endfunction
