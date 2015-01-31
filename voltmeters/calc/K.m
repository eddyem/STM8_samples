function [u dmax] = K(U, ADU)
	adu = uint32(ADU);
	U = uint32(bitshift(U*1000,17));
	u = uint32(U/adu);
	dmax = 2*uint32(2^31/u);
endfunction
