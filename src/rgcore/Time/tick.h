#pragma once

namespace rgCore{ namespace Time {


typedef __int64 tick_t;


void tick_init();
void tick_final();


tick_t tick_get();


}}