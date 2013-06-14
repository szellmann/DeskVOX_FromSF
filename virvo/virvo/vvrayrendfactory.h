#pragma once

/*! \brief  factory function, can e. g. be called by dlsym() to create rayrend from within plugin
 */
extern "C" vvRenderer* createSoftRayRend(vvVolDesc* vd, vvRenderState const& rs);

