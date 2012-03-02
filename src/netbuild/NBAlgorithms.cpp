/****************************************************************************/
/// @file    NBAlgorithms.cpp
/// @author  Daniel Krajzewicz
/// @date    02. March 2012
/// @version $Id$
///
// 
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// Copyright (C) 2001-2012 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <sstream>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <utils/common/MsgHandler.h>
#include "NBEdge.h"
#include "NBNodeCont.h"
#include "NBNode.h"
#include "NBAlgorithms.h"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS


// ===========================================================================
// method definitions
// ===========================================================================
// ---------------------------------------------------------------------------
// NBTurningDirectionsComputer
// ---------------------------------------------------------------------------
void 
NBTurningDirectionsComputer::compute(NBNodeCont &nc) {
    for(std::map<std::string, NBNode*>::const_iterator i=nc.begin(); i!=nc.end(); ++i) {
        const std::vector<NBEdge*> &incoming = (*i).second->getIncomingEdges();
        const std::vector<NBEdge*> &outgoing = (*i).second->getOutgoingEdges();
        std::vector<Combination> combinations;
        for(std::vector<NBEdge*>::const_iterator j=outgoing.begin(); j!=outgoing.end(); ++j) {
            NBEdge *outedge = *j;
            for(std::vector<NBEdge*>::const_iterator k=incoming.begin(); k!=incoming.end(); ++k) {
                NBEdge* e = *k;
                if (e->getConnections().size()!=0 && !e->isConnectedTo(outedge)) {
                    // has connections, but not to outedge; outedge will not be the turn direction
                    //
                    // @todo: this seems to be needed due to legacy issues; actually, we could regard
                    //  such pairs, too, and it probably would increase the accuracy. But there is
                    //  no mechanism implemented, yet, which would avoid adding them as turnarounds though
                    //  no connection is specified.
                    continue;
                }
                SUMOReal angle = fabs(NBHelpers::relAngle(e->getAngle(*(*i).second), outedge->getAngle(*(*i).second)));
                if(angle<160) {
                    continue;
                }
                if(e->getFromNode()==outedge->getToNode()) {
                    // they connect the same nodes; should be the turnaround direction
                    // we'll assign a maximum number
                    //
                    // @todo: indeed, we have observed some pathological intersections
                    //  see "294831560" in OSM/adlershof. Here, several edges are connecting
                    //  same nodes. We have to do the angle check before...
                    //
                    // @todo: and well, there are some other as well, see plain import
                    //  of delphi_muenchen (elmar), intersection "59534191". Not that it would
                    //  be realistic in any means; we will warn, here.
                    angle += 360;
                }
                Combination c;
                c.from = e;
                c.to = outedge;
                c.angle = angle;
                combinations.push_back(c);
            }
        }
        // sort combinations so that the ones with the highest angle are at the begin
        std::sort(combinations.begin(), combinations.end(), combination_by_angle_sorter());
        std::set<NBEdge*> seen;
        bool haveWarned = false;
        for(std::vector<Combination>::const_iterator j=combinations.begin(); j!=combinations.end(); ++j) {
            if(seen.find((*j).from)!=seen.end() || seen.find((*j).to)!=seen.end() ) {
                // do not regard already set edges
                if((*j).angle>360&&!haveWarned) {
                    WRITE_WARNING("Ambiguity in turnarounds computation at node '" + (*i).first +"'.");
                    haveWarned = true;
                }
                continue;
            }
            // mark as seen
            seen.insert((*j).from);
            seen.insert((*j).to);
            // set turnaround information
            (*j).from->setTurningDestination((*j).to);
        }
    }
}


/****************************************************************************/

