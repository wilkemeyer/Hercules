#pragma once



namespace rgCore {
namespace Logging {

class coreLogger : public logger {
public:
	coreLogger();


	// Initializes/Finalizes the global Object
	static void init();
	static void final();
	static coreLogger *get();

protected:
	__int64 get_Tick();

private:
	static coreLogger *m_inst;


};


} }
