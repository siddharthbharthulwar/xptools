/*
 * Copyright (c) 2011, mroe.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef WED_XPLUGINOBJECT_H
#define WED_XPLUGINOBJECT_H

#include "WED_XPluginNode.h"
#include "WED_XPluginDrawObj.h"
#include "XPLMGraphics.h"

class WED_XPluginMgr;

class WED_XPluginObject : public WED_XPluginNode
{
public:

    WED_XPluginObject(WED_XPluginMgr * inRef,const vector<string>& inArgs);
    WED_XPluginObject(WED_XPluginMgr * inRef);
    virtual ~WED_XPluginObject();

    void                 Draw(bool isLit);
    int                  SetRessource(const string& inPath);
    void                 Update(const vector<string>& inArgs);
    void                 UpdatePos();
    void                 SceneryShift(){UpdatePos();}

    void                 AddObj(WED_XPluginDrawObj * inObj){mObjs.push_back(inObj);}
    unsigned int         GetObjCnt(){return mObjs.size();}
    WED_XPluginDrawObj * GetObjAt(unsigned int n){return mObjs.at(n);}

protected:
private:

    bool           mIsAGP;
    bool           mWantDraw;

    vector<WED_XPluginDrawObj *> mObjs;

    void          ClearObjs();
    static void   LoadObjectCB(const char * inPath,void * inRef);
    static int    DoParseAGP(const string& inPath,WED_XPluginMgr * inMgr,
                                              WED_XPluginObject * inObj);

};

#endif // WED_XPLUGINOBJECT_H
