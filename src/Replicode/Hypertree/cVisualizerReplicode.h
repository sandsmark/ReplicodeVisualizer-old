/*
 *  cVisualizerReplicode.h
 *  Visor
 *
 *  Created by mb on 7/15/10.
 *  Copyright 2010 Communicative Machines. All rights reserved.
 *
 */

#pragma once

#include <Visualizer/Hypertree/cVisualizerHypertree.h>

namespace Visor
{
    class cVisualizerReplicode : public cVisualizerHypertree
    {
    public:
        cVisualizerReplicode( Ogre::SceneManager *iScene ) : cVisualizerHypertree( iScene ) {}
        
        // IListenerComponent methods
        virtual void OnComponentCreated    ( IComponent *iComponent );
        virtual void OnComponentInitialized( IComponent *iComponent );
    };
}
