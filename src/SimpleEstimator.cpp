//
// Created by Nikolay Yakovets on 2018-02-01.
//
#include <chrono>
#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
}

std::vector<std::unordered_set<uint32_t> > inNode;
std::vector<std::unordered_set<uint32_t> > outNode;
std::vector<std::vector<uint32_t> > nodeTotal;

void SimpleEstimator::prepare() {
    inNode.resize(graph->getNoVertices());
    outNode.resize(graph->getNoVertices());
    nodeTotal.resize(graph->getNoVertices());


    for (int i = 0; i < graph->adj.size(); i++) {
        for (int j = 0; j < graph->adj[i].size(); j++) {

            int edge = graph->adj[i].data()[j].first;
            int to = graph->adj[i].data()[j].second;
            int from = i;


            inNode[edge].insert(to);
            outNode[edge].insert(from);

            nodeTotal[edge].emplace_back(to);
        }
    }
}

uint32_t T = 0;
uint32_t VR = 0;
uint32_t VS = 0;
uint32_t nrPases = 0;
uint8_t loops = 0;

void initialize() {
    T = 0;
    VR = 0;
    VS = 0;
    nrPases = 0;
    loops = 0;
}

std::shared_ptr<SimpleGraph> SimpleEstimator::estimate_aux(RPQTree *q) {

    // estimate according to the AST bottom-up

    if (q->isLeaf()) {

        // project out the label in the AST
        std::regex directLabel(R"((\d+)\+)");
        std::regex inverseLabel(R"((\d+)\-)");

        std::smatch matches;

        uint32_t label;
        bool inverse;

        if (std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if (std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        SimpleEstimator::calculate(label, inverse);

    }

    if (q->isConcat()) {
        loops++;
        // estimate the children
        auto leftGraph = SimpleEstimator::estimate_aux(q->left);
        auto rightGraph = SimpleEstimator::estimate_aux(q->right);

    }

    return nullptr;
}

void SimpleEstimator::calculate(uint32_t labell, bool inverse) {
    if (inverse) {
        T = nodeTotal[labell].size();
        VR = outNode[labell].size();
        VS = inNode[labell].size();
    } else {
        T = nodeTotal[labell].size();
        VR = inNode[labell].size();
        VS = outNode[labell].size();
    }

    auto tt = T * T;
    auto value1 = tt / VS;
    auto value2 = tt / VR;
    if (loops == 0) {
        nrPases += T;
    } else {
        nrPases += std::min(value1, value2);
    }


}

cardStat SimpleEstimator::estimate(RPQTree *q) {
    // perform your estimation here;
    initialize();
    auto res = estimate_aux(q);
    //return SimpleEstimator::computeStats(res);


    return cardStat {VR, nrPases, VS};
}

