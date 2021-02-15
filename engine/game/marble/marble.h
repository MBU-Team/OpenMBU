#ifndef _MARBLE_H_
#define _MARBLE_H_

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif

class Marble : public ShapeBase
{
private:
    typedef ShapeBase Parent;

public:
    DECLARE_CONOBJECT(Marble);
};

class MarbleData : public ShapeBaseData
{
private:
    typedef ShapeBaseData Parent;

public:
    DECLARE_CONOBJECT(MarbleData);
};

#endif // _MARBLE_H_
