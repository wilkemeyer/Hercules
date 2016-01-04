#pragma once

namespace rgCore { namespace gui {

class infobox {
public:
	/**
	* For Simple Counter Values
	*
	* @note will be called in different threads, so the application has to ensure MT safetiness
	*       provided callback will be called directly by UI Thread
	*/
	typedef void(__stdcall *tCounterCallback)(size_t *currentValue, size_t *peak);
	void registerCounter(const char *name, tCounterCallback cbfn);


	/**
	* For Simple Pool-like Counter Values
	*
	* @note will be called in different threads, so the application has to ensure MT safetiness
	*       provided callback will be called directly by UI Thread
	*/
	typedef void(__stdcall *tPoolCounterCallback)(size_t *total, size_t *free, size_t *peak);
	void registerPoolCounter(const char *name, tPoolCounterCallback cbfn);



private:
	friend class win_ui;

	static void init();
	static void final();
	static void draw(win_ui *inst, HDC hdc);





};

	
} }
