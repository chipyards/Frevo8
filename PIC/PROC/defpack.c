#pragma romdata jln_txt4

// recette par defaut
rom unsigned char defpack[] = {
0x00, 0x06, 0x00,	// recette vide minimale
0x00, 0x00, 0x00	// CRC = 3E825403
};

/* juste un exemple de codage...
3, 42, 0,		// 3 steps occupant 42 bytes
0x00, 1, 20,		// step 1
0x01, 30, 0,		// duree 30 s
0x02, 0x20, 0x55,	// vannes
0x10, 0x01, 0xF0,	// MFC0, SV
0x00, 2, 21,		// step 2 - sans duree ==> pause
0x02, 0x40, 0x11,	// vannes
0x00, 5, 22,		// step 5
0x01, 0x01, 0x02,	// duree 513 s
0x02, 0x53, 0x19,	// vannes
0x40, 0x13, 0x01,	// MFC3, SV
0x70, 0x01, 0x01,	// TEM2, SV
0x81, 0xaa, 0xbb,	// fre, SVmi
0x83, 0x03, 0x01	// fre, flags et stogo
};
*/
