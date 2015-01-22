#ifndef RENDERERIMPLPK_H
#define RENDERERIMPLPK_H

#include "rendererinterface.h"

class RendererImplPK
  : public RendererInterface
{
public:

    explicit RendererImplPK(core::TimerArray&, unsigned int);
    ~RendererImplPK();

    virtual void render(unsigned int, bool = false, bool = false,
                        bool = false, bool = false);

private:

    virtual void initShaders();
    virtual void createVoxelList(bool);
    virtual void buildVoxelTree(bool);

};

#endif // RENDERERIMPLPK_H
