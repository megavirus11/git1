//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include <set>
#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

SimpleEvaluator::SimpleEvaluator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
    est = nullptr; // estimator not attached by default
}

void SimpleEvaluator::attachEstimator(std::shared_ptr<SimpleEstimator> &e) {
    est = e;
}

std::vector<std::shared_ptr<SimpleGraph>> graphCache;
std::vector<std::shared_ptr<SimpleGraph>> graphCacheInverse;

void SimpleEvaluator::prepare() {
    // if attached, prepare the estimator
    //if (est != nullptr) est->prepare();

    for (int i = 0; i < graph->getNoLabels(); ++i) {
        graphCache.emplace_back(SimpleEvaluator::project(static_cast<uint32_t>(i), false, graph));
        graphCacheInverse.emplace_back(SimpleEvaluator::project(static_cast<uint32_t>(i), true, graph));
    }

    //prepare other things here.., if necessary

}

cardStat SimpleEvaluator::computeStats(std::shared_ptr<SimpleGraph> &g) {

    cardStat stats{};

    /*for(int source = 0; source < g->getNoVertices(); source++) {
        if(!g->adj[source].empty()) stats.noOut++;
    }*/

    stats.noPaths = g->getNoDistinctEdges();

    /*for(int target = 0; target < g->getNoVertices(); target++) {
        if(!g->reverse_adj[target].empty()) stats.noIn++;
    }*/

    return stats;
}

std::shared_ptr<SimpleGraph>
SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());

    //if(!inverse) {
    // going forward
    for (uint32_t source = 0; source < in->getNoVertices(); source++) {
        for (auto labelTarget : in->adj[source]) {
            auto label = labelTarget.first;
            if (label == projectLabel) {
                auto target = labelTarget.second;
                if (!inverse) {
                    out->addEdge(source, target, label);
                } else {
                    out->addEdge(target, source, label);
                }
            }
        }
    }
    /*} else {
        // going backward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->reverse_adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    }*/

    return out;
}

std::shared_ptr<SimpleGraph>
SimpleEvaluator::join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    auto out = std::make_shared<SimpleGraph>(left->getNoVertices());
    out->setNoLabels(1);

    for (uint32_t leftSource = 0; leftSource < left->getNoVertices(); leftSource++) {
        for (auto labelTarget : left->adj[leftSource]) {

            int leftTarget = labelTarget.second;
            // try to join the left target with right source
            for (auto rightLabelTarget : right->adj[leftTarget]) {

                auto rightTarget = rightLabelTarget.second;
                out->addEdge(leftSource, rightTarget, 0);

            }
        }
    }
    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluate_aux(RPQTree *q) {

    // evaluate according to the AST bottom-up

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
        if (!inverse) {
            return graphCache[label];
        }
        else {
            return graphCache[label];
        }
    }

    if (q->isConcat()) {
        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);
        // join left with right
        return SimpleEvaluator::join(leftGraph, rightGraph);

    }

    return nullptr;
}

/*std::vector<std::shared_ptr<SimpleGraph>> graphlist;*/

/*std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluate_aux_nojoin(RPQTree *q) {

    // evaluate according to the AST bottom-up

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

        auto out = SimpleEvaluator::project(label, inverse, graph);
        graphlist.emplace_back(out);
        return out;
    }

    if (q->isConcat()) {

        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux_nojoin(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux_nojoin(q->right);
    }

    return nullptr;
}*/

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
    /*graphlist.clear();*/
    auto res = evaluate_aux(query);
    /*auto abc = evaluate_aux_nojoin(query);
    auto res = evaluate_join_list(graphlist);*/
    return SimpleEvaluator::computeStats(res);
}

/*std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluate_join_list(std::vector<std::shared_ptr<SimpleGraph>> &vector) {
    if (vector.size() == 1)
        return vector[0];
    if (vector.size() == 2)
        return SimpleEvaluator::join(vector[0], vector[1]);


    while (vector.size() > 2) {
        float min = 9999999999999.0f;
        auto indexi = -1;
        auto indexj = -1;
        for (int i = 0; i < vector.size(); i++) {
            for (int j = i + 1; j < vector.size(); j++) {
                auto a = vector[i]->getNoEdges();
                auto b = vector[i]->getNoDistinctEdges();

                auto c = vector[j]->getNoEdges();
                auto d = vector[j]->getNoDistinctEdges();

                float p1 = (float) vector[i]->getNoEdges() / (float) vector[i]->getNoDistinctEdges();
                float p2 = (float) vector[j]->getNoEdges() / (float) vector[j]->getNoDistinctEdges();
                float result = std::min((float) vector[i]->getNoEdges() * p2, (float) vector[j]->getNoEdges() * p1);
                if (result < min) {
                    min = result;
                    indexi = i;
                    indexj = j;
                }
            }
        }
        vector.emplace_back(SimpleEvaluator::join(vector[indexi], vector[indexj]));
        vector.erase(vector.begin() + indexi);
        vector.erase(vector.begin() + indexj-1);

        if(vector.back()->getNoDistinctEdges()==0)
            vector.erase(vector.end());
    }
    return SimpleEvaluator::join(vector[0], vector[1]);
}*/
