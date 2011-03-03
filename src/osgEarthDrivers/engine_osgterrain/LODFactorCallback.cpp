/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
* Copyright 2008-2011 Pelican Mapping
* http://osgearth.org
*
* osgEarth is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "LODFactorCallback"

#include <osg/Math>
#include <osg/PagedLOD>
#include <osg/StateSet>
#include <osg/Uniform>
#include <osgUtil/CullVisitor>

namespace osgEarth
{
namespace Drivers
{
void LODFactorCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    osg::PagedLOD* lod =  static_cast<osg::PagedLOD*>(node);
    osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
    osg::LOD::RangeMode rangeMode = lod->getRangeMode();
    float requiredRange = 0.0f;
    float rangeFactor = 0.0f;
    const osg::LOD::RangeList& rangeList = lod->getRangeList();
    if (rangeMode == osg::LOD::DISTANCE_FROM_EYE_POINT)
    {
        requiredRange = cv->getDistanceToViewPoint(lod->getCenter(), true);
    }
    else if (cv->getLODScale() > 0.0f)
    {
        requiredRange = cv->clampedPixelSize(lod->getBound()) / cv->getLODScale();
    }
    else
    {
        // The comment in osg/PagedLOD.cpp says that this algorithm
        // finds the highest res tile, but it actually finds the
        // lowest res tile!
        for (osg::LOD::RangeList::const_iterator itr = rangeList.begin(),
                 end = rangeList.end();
             itr != end;
             ++itr)
            requiredRange = osg::maximum(requiredRange, itr->first);
    }
    // We're counting on only finding one valid LOD, unlike the
    // general OSG behavior.
    for (unsigned i = 0; i < rangeList.size(); ++i)
    {
        if (rangeList[i].first <= requiredRange && requiredRange < rangeList[i].second)
        {
            if (i >= lod->getNumChildren())
            {
                // The displayed node is already less detailed than
                // desired, so its LOD factor is biased all the way to
                // 1.
                rangeFactor = 1.0f;
            }
            else
            {
                rangeFactor = 1.0f - ((requiredRange - rangeList[i].first)
                                      / (rangeList[i].second - rangeList[i].first));
            }
            break;
        }
    }
    osg::ref_ptr<osg::Uniform> ufact
        = new osg::Uniform("osgearth_LODRangeFactor", rangeFactor);
    osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
    ss->addUniform(ufact.get());
    cv->pushStateSet(ss.get());
    traverse(node, nv);
    cv->popStateSet();
}
}
}
