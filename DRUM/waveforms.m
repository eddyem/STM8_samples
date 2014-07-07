function waveforms(FN, nm)
% Prints on stdout 16 values for waveform bank
%   FN - array with 16 values of Vout (Vmin..Vmax), will be normalized
%   nm - name of array
	MIN = min(FN); MAX = max(FN);
	FN = (FN - MIN) / (MAX - MIN);
	VAL = round(FN * 16);
	printf("static const U8 %s[16] = {", nm)
	for i = 1:15; printf("%d, ", VAL(i)); endfor;
	printf("%d};\n", VAL(16));
	plot(VAL, 'o');
endfunction
