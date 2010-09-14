/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef SEARCHENGINE_H_
#define SEARCHENGINE_H_

#include <climits>
#include <deque>

#include "BinaryHeap.h"
#include "../DataStructures/PhantomNodes.h"
#include "../typedefs.h"

struct _HeapData {
    NodeID parent;
    _HeapData( NodeID p ) : parent(p) { }
};

typedef BinaryHeap< NodeID, int, int, _HeapData, DenseStorage< NodeID, unsigned > > _Heap;

template<typename EdgeData, typename GraphT, typename NodeHelperT = NodeInformationHelpDesk>
class SearchEngine {
private:
    const GraphT * _graph;
    inline double absDouble(double input) { if(input < 0) return input*(-1); else return input;}
public:
    SearchEngine(GraphT * g, NodeHelperT * nh) : _graph(g), nodeHelpDesk(nh) {}
    ~SearchEngine() {}

    inline const void getNodeInfo(NodeID id, _Coordinate& result) const
    {
        result.lat = nodeHelpDesk->getLatitudeOfNode(id);
        result.lon = nodeHelpDesk->getLongitudeOfNode(id);
    }

    unsigned int numberOfNodes() const
    {
        return nodeHelpDesk->getNumberOfNodes();
    }

    unsigned int ComputeRoute(PhantomNodes * phantomNodes, vector<NodeID> * path, _Coordinate& startCoord, _Coordinate& targetCoord)
    {
        bool onSameEdge = false;
        _Heap * _forwardHeap = new _Heap(nodeHelpDesk->getNumberOfNodes());
        _Heap * _backwardHeap = new _Heap(nodeHelpDesk->getNumberOfNodes());
        NodeID middle = ( NodeID ) 0;
        unsigned int _upperbound = std::numeric_limits<unsigned int>::max();

        if(phantomNodes->startNode1 == UINT_MAX || phantomNodes->startNode2 == UINT_MAX)
            return _upperbound;

        if( (phantomNodes->startNode1 == phantomNodes->targetNode1 && phantomNodes->startNode2 == phantomNodes->targetNode2 ) )
        {
            EdgeID currentEdge = _graph->FindEdge( phantomNodes->startNode1, phantomNodes->startNode2 );
            if(currentEdge == UINT_MAX)
                currentEdge = _graph->FindEdge( phantomNodes->startNode2, phantomNodes->startNode1 );
            if(currentEdge != UINT_MAX && _graph->GetEdgeData(currentEdge).forward && phantomNodes->startRatio < phantomNodes->targetRatio)
            { //upperbound auf kantenlänge setzen
                //                cout << "start and target on same edge" << endl;
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if (currentEdge != UINT_MAX && !_graph->GetEdgeData(currentEdge).backward) {
                EdgeWeight w = _graph->GetEdgeData( currentEdge ).distance;
                _forwardHeap->Insert(phantomNodes->startNode2, absDouble(w*phantomNodes->startRatio), phantomNodes->startNode2);
                _backwardHeap->Insert(phantomNodes->startNode1, absDouble(w-w*phantomNodes->startRatio), phantomNodes->startNode1);
            } else if (currentEdge != UINT_MAX && _graph->GetEdgeData(currentEdge).backward)
            {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            }
        } else {
            if(phantomNodes->startNode1 != UINT_MAX)
            {
                EdgeID forwardEdge = _graph->FindEdge( phantomNodes->startNode1, phantomNodes->startNode2);
                if(forwardEdge == UINT_MAX)
                    forwardEdge = _graph->FindEdge( phantomNodes->startNode2, phantomNodes->startNode1 );
                if(forwardEdge != UINT_MAX && _graph->GetEdgeData(forwardEdge).forward )
                { //insert forward edge (coord, s1) in forward heap;
                    EdgeWeight w = _graph->GetEdgeData(forwardEdge ).distance;
                    _forwardHeap->Insert(phantomNodes->startNode1, absDouble(w*phantomNodes->startRatio), phantomNodes->startNode1);
                }
                EdgeID backEdge = _graph->FindEdge( phantomNodes->startNode2, phantomNodes->startNode1);
                if(backEdge == UINT_MAX)
                    backEdge = _graph->FindEdge( phantomNodes->startNode1, phantomNodes->startNode2 );
                if(backEdge != UINT_MAX && _graph->GetEdgeData(backEdge).backward )
                { //insert forward edge (coord, s2) in forward heap;
                    EdgeWeight w = _graph->GetEdgeData( backEdge ).distance;
                    _forwardHeap->Insert(phantomNodes->startNode2, absDouble(w-w*phantomNodes->startRatio), phantomNodes->startNode2);
                }
            }
            if(phantomNodes->targetNode1 != UINT_MAX)
            {
                EdgeID forwardEdge = _graph->FindEdge( phantomNodes->targetNode1, phantomNodes->targetNode2);
                if(forwardEdge == UINT_MAX)
                    forwardEdge = _graph->FindEdge( phantomNodes->targetNode2, phantomNodes->targetNode1 );

                if(forwardEdge != UINT_MAX && _graph->GetEdgeData(forwardEdge).forward )
                { //insert forward edge (coord, s1) in forward heap;
                    EdgeWeight w = _graph->GetEdgeData( forwardEdge ).distance;
                    _backwardHeap->Insert(phantomNodes->targetNode1, absDouble(w * phantomNodes->targetRatio), phantomNodes->targetNode1);
                }
                EdgeID backwardEdge = _graph->FindEdge( phantomNodes->targetNode2, phantomNodes->targetNode1);
                if(backwardEdge == UINT_MAX)
                    backwardEdge = _graph->FindEdge( phantomNodes->targetNode1, phantomNodes->targetNode2 );
                if(backwardEdge != UINT_MAX && _graph->GetEdgeData( backwardEdge ).backward )
                { //insert forward edge (coord, s2) in forward heap;
                    EdgeWeight w = _graph->GetEdgeData( backwardEdge ).distance;
                    _backwardHeap->Insert(phantomNodes->targetNode2, absDouble(w - w * phantomNodes->targetRatio), phantomNodes->targetNode2);
                }
            }
        }

        while(_forwardHeap->Size() + _backwardHeap->Size() > 0)
        {
            if ( _forwardHeap->Size() > 0 ) {
                _RoutingStep( _forwardHeap, _backwardHeap, true, &middle, &_upperbound );
            }
            if ( _backwardHeap->Size() > 0 ) {
                _RoutingStep( _backwardHeap, _forwardHeap, false, &middle, &_upperbound );
            }
        }

        if ( _upperbound == std::numeric_limits< unsigned int >::max() || onSameEdge )
            return _upperbound;

        NodeID pathNode = middle;
        deque< NodeID > packedPath;

        while ( pathNode != phantomNodes->startNode1 && pathNode != phantomNodes->startNode2 ) {
            pathNode = _forwardHeap->GetData( pathNode ).parent;
            packedPath.push_front( pathNode );
        }

        packedPath.push_back( middle );
        pathNode = middle;

        while ( pathNode != phantomNodes->targetNode2 && pathNode != phantomNodes->targetNode1 ) {
            pathNode = _backwardHeap->GetData( pathNode ).parent;
            packedPath.push_back( pathNode );
        }

        // push start node explicitely
        path->push_back(packedPath[0]);
        for(deque<NodeID>::size_type i = 0; i < packedPath.size()-1; i++)
        {
            _UnpackEdge(packedPath[i], packedPath[i+1], path);
        }

        packedPath.clear();
        delete _forwardHeap;
        delete _backwardHeap;

        return _upperbound/10;
    }

    unsigned int ComputeDistanceBetweenNodes(NodeID start, NodeID target)
    {
        _Heap * _forwardHeap = new _Heap(_graph->GetNumberOfNodes());
        _Heap * _backwardHeap = new _Heap(_graph->GetNumberOfNodes());
        NodeID middle = ( NodeID ) 0;
        unsigned int _upperbound = std::numeric_limits<unsigned int>::max();

        _forwardHeap->Insert(start, 0, start);
        _backwardHeap->Insert(target, 0, target);

        while(_forwardHeap->Size() + _backwardHeap->Size() > 0)
        {
            if ( _forwardHeap->Size() > 0 ) {
                _RoutingStep( _forwardHeap, _backwardHeap, true, &middle, &_upperbound );
            }
            if ( _backwardHeap->Size() > 0 ) {
                _RoutingStep( _backwardHeap, _forwardHeap, false, &middle, &_upperbound );
            }
        }
        delete _forwardHeap;
        delete _backwardHeap;
        return _upperbound;
    }

    inline unsigned int findNearestNodeForLatLon(const _Coordinate& coord, _Coordinate& result) const
    {
        nodeHelpDesk->findNearestNodeIDForLatLon( coord, result );
    }

    inline bool FindRoutingStarts(const _Coordinate start, const _Coordinate target, PhantomNodes * routingStarts)
    {
        nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
    }
private:
    NodeHelperT * nodeHelpDesk;

    void _RoutingStep(_Heap * _forwardHeap, _Heap *_backwardHeap, const bool& forwardDirection, NodeID * middle, unsigned int * _upperbound)
    {
        const NodeID node = _forwardHeap->DeleteMin();
        const unsigned int distance = _forwardHeap->GetKey( node );
        if ( _backwardHeap->WasInserted( node ) ) {
            const unsigned int newDistance = _backwardHeap->GetKey( node ) + distance;
            if ( newDistance < *_upperbound ) {
                *middle = node;
                *_upperbound = newDistance;
            }
        }
        if ( distance > *_upperbound ) {
            _forwardHeap->DeleteAll();
            return;
        }
        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const NodeID to = _graph->GetTarget(edge);
            const int edgeWeight = _graph->GetEdgeData(edge).distance;

            assert( edgeWeight > 0 );
            const int toDistance = distance + edgeWeight;

            if(forwardDirection ? _graph->GetEdgeData(edge).forward : _graph->GetEdgeData(edge).backward )
            {
                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap->WasInserted( to ) )
                {
                    _forwardHeap->Insert( to, toDistance, node );
                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forwardHeap->GetKey( to ) ) {
                    _forwardHeap->GetData( to ).parent = node;
                    _forwardHeap->DecreaseKey( to, toDistance );
                    //new parent
                }
            }
        }
    }

    bool _UnpackEdge( const NodeID source, const NodeID target, std::vector< NodeID >* path ) {
        assert(source != target);
        //find edge first.
        typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
        EdgeWeight smallestWeight = UINT_MAX;
        for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(source); eit < _graph->EndEdges(source); eit++)
        {
            const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
            {
                if(_graph->GetTarget(eit) == target && weight < smallestWeight && _graph->GetEdgeData(eit).forward)
                {
                    smallestEdge = eit; smallestWeight = weight;
                }
            }
        }
        if(smallestEdge == SPECIAL_EDGEID)
        {
            for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(target); eit < _graph->EndEdges(target); eit++)
            {
                const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
                {
                    if(_graph->GetTarget(eit) == source && weight < smallestWeight && _graph->GetEdgeData(eit).backward)
                    {
                        smallestEdge = eit; smallestWeight = weight;
                    }
                }
            }
        }

        assert(smallestWeight != SPECIAL_EDGEID); //no edge found. This should not happen at all!

        const EdgeData ed = _graph->GetEdgeData(smallestEdge);
        if(ed.shortcut)
        {//unpack
            const NodeID middle = ed.middle;
            _UnpackEdge(source, middle, path);
            _UnpackEdge(middle, target, path);
            return false;
        } else {
            assert(!ed.shortcut);
            path->push_back(target);
            return true;
        }
    }
};

#endif /* SEARCHENGINE_H_ */
