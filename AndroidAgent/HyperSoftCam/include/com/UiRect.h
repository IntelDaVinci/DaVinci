#ifndef UIRECT_H_
#define UIRECT_H_

namespace android {

struct UiRect
{
    int x;
    int y;
    int width;
    int height;

    UiRect()
    {
        x = 0;
        y = 0;
        width = 0;
        height = 0;
    }

    int64_t area()
    {
        return width * height;
    }

    bool containPoint(int px, int py)
    {
        if(px >= x && px <= x + width 
           && py >= y && py <= y + height)
            return true;
        else
            return false;
    }

    bool containRect(UiRect rect, int edge = 2)
    {
        if(x > rect.x)
	    return false;

        if(y > rect.y)
            return false;
            
        if(x + width + edge < rect.x + rect.width)
            return false;

        if(y + height + edge < rect.y + rect.height)
            return false;

        return true;
    }

    bool equals(UiRect rect)
    {
	if(x != rect.x)
      	    return false;
		
	if(y != rect.y)
	    return false;

	if(width != rect.width)
	    return false;

	if(height != rect.height)
	    return false;

	return true;
    }
};

};
#endif
