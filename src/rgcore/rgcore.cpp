#include "stdafx.h"

using namespace rgCore;
using namespace rgCore::Time;

void rgCore_init(){

	
	roalloc_init();
	tick_init();

}//end: rgCore_init()


void rgCore_final() {

	tick_final();
	roalloc_final();
	


}//end: rgCore_final()
