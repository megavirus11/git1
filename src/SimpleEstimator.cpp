//
// Created by Nikolay Yakovets on 2018-02-01.
//
#include <chrono>
#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    // works only with SimpleGraph
    graph = g;
}

std::vector< std::unordered_set <uint32_t> > inNode;
std::vector< std::unordered_set <uint32_t> > outNode;
std::vector< std::vector <uint32_t> > inNodeTotal;
std::vector< std::vector <uint32_t> > outNodeTotal;



uint32_t pass=0;

void SimpleEstimator::prepare() {
    inNode.resize(graph->getNoVertices());
    outNode.resize(graph->getNoVertices());
    inNodeTotal.resize(graph->getNoVertices());
    outNodeTotal.resize(graph->getNoVertices());


        for (int i = 0; i < graph->adj.size(); i++) {
            for (int j = 0; j < graph->adj[i].size(); j++) {

                int edge = graph->adj[i].data()[j].first;
                int to = graph->adj[i].data()[j].second;
                int from = i;


                inNode[edge].insert(to);
                outNode[edge].insert(from);

                inNodeTotal[edge].emplace_back(to);
                outNodeTotal[edge].emplace_back(from);
            }
        }
}

uint32_t TR=0;
uint32_t TS=0;
uint32_t VR=0;
uint32_t VS=0;
uint32_t nrPases=0;
uint32_t noOut=0;
uint32_t noIn=0;

void initialize()
{
    TR=0;
    TS=0;
    VR=0;
    VS=0;
    nrPases=0;
    noOut=0;
    noIn=0;
}

std::shared_ptr<SimpleGraph> SimpleEstimator::estimate_aux(RPQTree *q) {

    // estimate according to the AST bottom-up



    if(q->isLeaf()) {
        // project out the label in the AST
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;

        uint32_t label;
        bool inverse;

        if(std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        SimpleEstimator::calculate(label, inverse);

    }

    if(q->isConcat()) {
        // estimate the children
        auto leftGraph = SimpleEstimator::estimate_aux(q->left);
        auto rightGraph = SimpleEstimator::estimate_aux(q->right);

    }

    return nullptr;
}

void SimpleEstimator::calculate(uint32_t labell, bool inverse) {
    if(inverse)
    {
        TR = outNodeTotal[labell].size();
        VR = outNode[labell].size();
        noIn = VR;
        TS = inNodeTotal[labell].size();
        VS = inNode[labell].size();
        noOut = VS;
    }
    else {

        TR = inNodeTotal[labell].size();
        VR = inNode[labell].size();
        noIn = VR;
        TS = outNodeTotal[labell].size();
        VS = outNode[labell].size();
        noOut = VS;
    }

    auto tt=TR*TS;
        auto value1 = tt/VS;
        auto value2 = tt/VR;
    if(nrPases == 0)
    {
        nrPases+=TR;
    }
    else{
        nrPases+=std::min(value1, value2);
    }

}

cardStat SimpleEstimator::estimate(RPQTree *q) {
    // perform your estimation here;
    initialize();
    auto res = estimate_aux(q);
    //return SimpleEstimator::computeStats(res);


    return cardStat {noOut, nrPases, noIn};
}






