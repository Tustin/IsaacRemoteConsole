//http://www.progamercity.net/c-code/377-c-signature-scanner.html

bool DataCompare(const BYTE* bData, const BYTE* bMask, int nLength)
{
	for (int i = 0; i < nLength; i++) { //search the whole length

		if ((bData[i] != bMask[i]) && (bMask[i] != 0x99)) //if they don't match & the mask is not 0x99

			return false; //they are not equal, return false

	}

	return true; //they are equal, return true
}



//function to search for signature

DWORD FindAddress(BYTE *bMask, int nLength, DWORD dwBaseAddress, DWORD dwLength)
{
	for (DWORD i = 0; i < (dwLength - nLength); i++) //while we're searching

		if (DataCompare((BYTE*)(dwBaseAddress + i), bMask, nLength)) //compare bytes

			return (DWORD)(dwBaseAddress + i); //address found! return it

	return 0; //no address found, return nothing

}