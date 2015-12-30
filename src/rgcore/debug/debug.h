#pragma once

typedef struct DebugInfo{
		size_t address,
				line;
			
		char	file[0xff],
				func[0xff];

} DebugInfo;

/**
	* <summary>	Gets Debug information for the given address
	* 				Such as line and file
	* 				Note that you dont have to free the returned struct. </summary>
	*
	* <remarks>	Wilkemeyer, 09.07.2011. </remarks>
	*
	* <param name="Address">	[in] If non-null, the address. </param>
	*
	* <returns>	Pointer to DebugInfo record, do - not - free the result! </returns>
	*/
DebugInfo *debugInfoGet(void *Address);


// Used for GUI Subsystem.
void debug_draw_BSOD(void *hdc);


bool debug_init();
void debug_final();
