enum ConsoleState {
	Closed,
	Open = 2,
	Opening = 4,
};
enum BufferType {
	Local = 0xf, //command buffer should be read directly from the structure
	Pointer = 0x1f //the command buffer should be read as a pointer (this is what I set it to so it parses my command from my pointer)
};
