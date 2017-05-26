#include "com/WindowBarParser.h"
#include "utils/KeyedVector.h"

namespace android {

    WindowBarParser::WindowBarParser()
    {
        AUTO_LOG();
    }

    WindowBarParser::~WindowBarParser()
    {
        AUTO_LOG();
    }

    UiRect WindowBarParser::ParseBarInfo(String8 line, String8 key)
    {
        AUTO_LOG();

        Vector<String8> items = StringUtil::split(line, ' ');
	if(items.size() < 2)
     	    return UiRect();
	
	String8 keyStr = items[1];
	if(!StringUtil::startsWith(keyStr, key.string()))
	    return UiRect();

	Vector<String8> rectItems = StringUtil::split(keyStr, '=');
        assert(rectItems.size() >= 2);
	String8 value = rectItems[1];
            
        UiRect rect;
	size_t begPos = 1;
        ssize_t pos = value.find(",", begPos);

	if (pos != -1)
	{
	    String8 intStr = StringUtil::substr(value, begPos, pos - begPos);
       	    rect.x = atoi(intStr);
	    begPos = pos + 1;
            pos = value.find("]", begPos);

	    if (pos != -1)
	    {
		intStr = StringUtil::substr(value, begPos, pos - begPos);
		rect.y = atoi(intStr);
		begPos = pos + 2;
				
		pos = value.find(",", begPos);
		if (pos != -1)
                {
	    	    intStr = StringUtil::substr(value, begPos, pos - begPos);
		    rect.width = atoi(intStr) - rect.x;
		    begPos = pos + 1;
                    
                    pos = value.find("]", begPos);
		    if (pos != -1)
                    {
		        intStr = StringUtil::substr(value, begPos, pos - begPos);
		        rect.height = atoi(intStr) - rect.y;
                        begPos = pos + 1;
		    }
		}                                
	    }                          
	}

        return rect;
    }

    void WindowBarParser::ParseInfo(char* region, UiRect& sb, UiRect& nb, int& kb, UiRect& fw, int& popup, String8 dumpFile)
    {
	AUTO_LOG();

	kb = 0;
        bool hasSb = false, hasNb = false, hasKb = false, hasWd = false;
        bool hasFinalSb = false, hasFinalNb = false, hasFinalFw = false;
        kb = 0, popup = 0;
	KeyedVector<String8, UiRect> windows;
	String8 windowKey("Window #");
        String8 windowIndex, focusWindowIndex;
        const char *d = "\n";
        char* p = strtok(region, d);
        int window_fd = open(dumpFile.string(),
                O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	char buffer[4096] = {0};
	int len = 0;
         
        while(p)
        {
            len = sprintf(buffer, "%s\n", p);
            write(window_fd, buffer, len);
            memset(buffer, 0, sizeof(buffer));

            String8 trimLine = StringUtil::trimLeft(String8(p));

            if(trimLine.length() == 0)
      	    {
                p = strtok(NULL, d);
                continue;
	    }
            
            if (StringUtil::startsWith(trimLine, windowKey.string()) && StringUtil::endsWith(trimLine, "NavigationBar}:"))
            {
                hasNb = true;
                p = strtok(NULL, d);
                continue;
            }
            if (StringUtil::startsWith(trimLine, windowKey.string()) && StringUtil::endsWith(trimLine, "StatusBar}:"))
            {
                hasSb = true;
                p = strtok(NULL, d);
                continue;
            }
            if (StringUtil::startsWith(trimLine, windowKey.string()) && StringUtil::endsWith(trimLine, "InputMethod}:"))
            {
                hasKb = true;
                p = strtok(NULL, d);
                continue;
            }
            if (StringUtil::startsWith(trimLine, windowKey.string()) && trimLine.find("Window{") != -1)
            {
                Vector<String8> items = StringUtil::split(trimLine, ' ');
		assert(items.size() == 5);
		String8 windowStr = items[2];
                Vector<String8> windowItems = StringUtil::split(windowStr, '{');
		assert(windowItems.size() == 2);
		windowIndex = windowItems[1];
		hasWd = true;
                p = strtok(NULL, d);
                continue;
            }
            if (StringUtil::startsWith(trimLine, "mCurrentFocus="))
            {
                Vector<String8> items = StringUtil::split(trimLine, ' ');
		assert(items.size() == 3);
		String8 windowStr = items[0];
                Vector<String8> windowItems = StringUtil::split(windowStr, '{');
		assert(windowItems.size() == 2);
		focusWindowIndex = windowItems[1];

                if(windows.indexOfKey(focusWindowIndex) >= 0)
		{
		    fw = windows.valueFor(focusWindowIndex);
		    hasFinalFw = true;
		}
                if(trimLine.find("PopupWindow:") != -1)
                {
                    popup = 1;
                }
		break;
	    }
            if (hasNb)
            {
                if (StringUtil::startsWith(trimLine, "mHasSurface="))
                {
	    	    if(StringUtil::endsWith(trimLine, "isReadyForDisplay()=true"))
		    {
	                hasNb = false;
    	                hasFinalNb = true;
        	        nb = ParseBarInfo(trimLine, String8("mShownFrame"));
            	    	p = strtok(NULL, d);
                        continue;
		    }
		    else
		    {
	                hasNb = false;
    	                hasFinalNb = false;
            	    	p = strtok(NULL, d);
                        continue;
		    }
                }
            }
            else if (hasSb)
            {
                if (StringUtil::startsWith(trimLine, "mHasSurface="))
                {
                    if(StringUtil::endsWith(trimLine, "isReadyForDisplay()=true"))
		    {
	                hasSb = false;
    	                hasFinalSb = true;
        	        sb = ParseBarInfo(trimLine, String8("mShownFrame"));
	                p = strtok(NULL, d);
                        continue;
		    }
		    else
    		    {
	                hasSb = false;
    	                hasFinalSb = false;
	                p = strtok(NULL, d);
                        continue;
		    }
                }   
            }
    	    else if (hasKb)
            {
                if (StringUtil::startsWith(trimLine, "mHasSurface="))
                {
                    if(StringUtil::endsWith(trimLine, "isReadyForDisplay()=true"))
	   	    {
	                hasKb = false;
    	                kb = 1;
	    	        p = strtok(NULL, d);
            	        continue;
		    }
		    else
		    {
	                hasKb = false;
    	                kb = 0;
	    	        p = strtok(NULL, d);
            	        continue;
		    }
                }
            }
	    else if (hasWd)
            {
                if (StringUtil::startsWith(trimLine, "content=["))
                {
                    hasWd = false;
                    UiRect wRect = ParseBarInfo(trimLine, String8("visible"));
	     	    windows.add(windowIndex, wRect);
	            p = strtok(NULL, d);
                    continue;
                }
            }
            p = strtok(NULL, d);
        }

        if (!hasFinalSb)
            sb = UiRect();

        if (!hasFinalNb)
            nb = UiRect();
        
        if (!hasFinalFw)
            fw = UiRect();
    }
}
