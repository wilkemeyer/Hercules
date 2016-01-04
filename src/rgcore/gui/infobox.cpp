#include "stdafx.h"
#include "../stdafx.h"


namespace rgCore {
namespace gui { 

typedef struct remoteCounterList {
	struct remoteCounterList *next;
	char *name;
	bool isPool;

	union {
		infobox::tPoolCounterCallback poolcb;
		infobox::tCounterCallback cb;
	};

} remoteCounterList;

static util::spinlock *g_remoteCounter_lock = NULL;
static remoteCounterList *g_remoteCounter_Begin = NULL;
static remoteCounterList *g_remoteCounter_End = NULL;


void infobox::init(){

	g_remoteCounter_lock = new util::spinlock();

}//end: init()


void infobox::final(){

	if(g_remoteCounter_lock != NULL){
		delete g_remoteCounter_lock;
		g_remoteCounter_lock = NULL;
	}

	{
		remoteCounterList *it, *itn;
		it = g_remoteCounter_Begin;
		while(1) {
			if(it == NULL)
				break;

			itn = it->next;

			rofree(it);

			it = itn;
		}
		g_remoteCounter_Begin = NULL;;
		g_remoteCounter_End = NULL;
	}

}//end: final()


void infobox::draw(win_ui *inst, HDC hdc){
	size_t numTotal, numFree, numPeak;
	HGDIOBJ prevobj;
	int y;

	// Draw Rect
	prevobj = SelectObject(hdc, inst->m_blackpen);
	Rectangle(hdc, 720, 20, 1000, 700);
	SelectObject(hdc, prevobj);

	//
	SetTextColor(hdc, RGB(0, 0, 0));

	// App Header (name + build date)
	y = 24;
#ifdef _DEBUG
	inst->TextOutf(hdc, false, 725, y, "rgCore - Runtime (Debug)");
#else
	TextOutf(hdc, false, 725, y, "rgServer - Runtime");
#endif
	y += 16;
	inst->TextOutf(hdc, false, 725, y, "Build Date: %s %s", __DATE__, __TIME__);
	y += 16;
	inst->TextOutf(hdc, false, 725, y, "Build on: %s\n", BUILD_HOSTNAME);
	y += 24;


	// Draw Uptime
	size_t up_days, up_hours, up_minutes, up_seconds;
	size_t CurTime = Time::tick_get()/1000;
	up_days = CurTime/86400;
	CurTime = CurTime%86400;
	up_hours= CurTime/3600;
	CurTime = CurTime%3600;
	up_minutes = CurTime/60;
	CurTime = CurTime%60;
	up_seconds = CurTime;

	inst->TextOutf(hdc, false, 725, y, "Uptime: %ud%02u:%02u:%02u", up_days, up_hours, up_minutes, up_seconds);
	y+=24;


	// Network Stats
	inst->TextOutf(hdc, true, 728, y, "Network");
	y+=16;

	{
		size_t numv4, numv6, peakv4, peakv6;
		rgNet_getSessionCounters(&numv4, &numv6, &peakv4, &peakv6);
		inst->TextOutf(hdc, false, 736, y, "Count: %u (v4: %u, v6: %u) Max: %u", numv4+numv6, numv4, numv6, peakv4+peakv6);
		y+=16;
	}
	network::PendingIOMgr_getCounters(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "Pending IO: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=16;

	network::netBuffer::getCounters64B(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "NB 64B: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=16;

	network::netBuffer::getCounters256B(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "NB 256B: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=16;

	network::netBuffer::getCounters1K(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "NB 1K: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=16;

	network::netBuffer::getCounters2K(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "NB 2K: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=16;

	network::netBuffer::getCounters4K(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "NB 4K: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=16;

	network::netBuffer::getCounters8K(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "NB 8K: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=24;

	// Draw Core Memory Pools
	inst->TextOutf(hdc, true, 728, y, "Core Memory Pool");
	y+=16;

	asyncDB_getTaskPoolCounters(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "AsyncDB Task: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=16;

	Time::timer::getCounters(&numTotal, &numFree, &numPeak);
	inst->TextOutf(hdc, false, 736, y, "Timer Nodes: %u (F:%u M:%u)", numTotal-numFree, numFree, numPeak);
	y+=24;

	//
	// Spacer
	//
	Rectangle(hdc, 720, y, 1000, y+1);

	// Appname / Title
	y +=4;
#ifdef _DEBUG
	inst->TextOutf(hdc, false, 725, y, "%s (Debug)", rgCore_getAppName());
#else
	inst->TextOutf(hdc, false, 725, y, "%s", g_AppName.c_str());
#endif
	y += 24;


	// Draw Application Provided Counters:
	// 
	g_remoteCounter_lock->lock();

	for(auto iter = g_remoteCounter_Begin; iter != NULL; iter = iter->next) {
		if(iter->isPool == true) {
			iter->poolcb(&numTotal, &numFree, &numPeak);
			inst->TextOutf(hdc, false, 736, y, "%s: %u (F:%u M:%u)", iter->name, numTotal-numFree, numFree, numPeak);
			y+=16;

		} else {
			size_t val, peakVal;
			iter->cb(&val, &peakVal);
			inst->TextOutf(hdc, false, 736, y, "%s: %u  Max: %u", iter->name, val, peakVal);
			y+= 16;
		}
	}

	g_remoteCounter_lock->unlock();

}//end: draw()


void infobox::registerCounter(const char *name, tCounterCallback cbfn) {
	size_t len = strlen(name);

	auto rc = (remoteCounterList*)roalloc(sizeof(remoteCounterList) + len + 1);

	rc->name = ((char*)rc) + sizeof(remoteCounterList);
	memcpy(rc->name, name, len+1);
	rc->isPool = false;
	rc->cb = cbfn;
	rc->next = NULL;


	g_remoteCounter_lock->lock();
	
	if(g_remoteCounter_Begin == NULL) {
		g_remoteCounter_Begin = rc;
		g_remoteCounter_End = rc;
	} else {
		g_remoteCounter_End->next = rc;
		g_remoteCounter_End = rc;
	}
	g_remoteCounter_lock->unlock();

}//end: registerCounter()


void infobox::registerPoolCounter(const char *name, tPoolCounterCallback cbfn){
	size_t len = strlen(name);

	auto rc = (remoteCounterList*)roalloc(sizeof(remoteCounterList) + len + 1);

	rc->name = ((char*)rc) + sizeof(remoteCounterList);
	memcpy(rc->name, name, len+1);
	rc->isPool = true;
	rc->poolcb = cbfn;
	rc->next = NULL;

	g_remoteCounter_lock->lock();

	if(g_remoteCounter_Begin == NULL) {
		g_remoteCounter_Begin = rc;
		g_remoteCounter_End = rc;
	} else {
		g_remoteCounter_End->next = rc;
		g_remoteCounter_End = rc;
	}

	g_remoteCounter_lock->unlock();

}//end: registerPoolCounter()



} } //end namespaces