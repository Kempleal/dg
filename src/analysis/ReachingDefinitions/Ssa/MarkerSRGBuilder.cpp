#include "analysis/ReachingDefinitions/Ssa/MarkerSRGBuilder.h"

using namespace dg::analysis::rd::ssa;

void MarkerSRGBuilder::writeVariable(const DefSite& var, NodeT *assignment) {
    // remember the last definition
    current_def[var][assignment->getBBlock()] = assignment;
}

MarkerSRGBuilder::NodeT *MarkerSRGBuilder::readVariable(const DefSite& var, BlockT *read) {
    auto& block_defs = current_def[var];
    auto it = block_defs.find(read);
    NodeT *assignment = nullptr;

    // find the last definition
    if (it != block_defs.end()) {
        assignment = it->second;
    } else {
        assignment = readVariableRecursive(var, read);
    }
    return assignment;
}

void MarkerSRGBuilder::addPhiOperands(const DefSite& var, NodeT *phi) {
    writeVariable(var, phi);

    phi->addDef(var, true);
    phi->addUse(var);

    for (BlockT *pred : phi->getBBlock()->predecessors()) {
        srg[readVariable(var, pred)].push_back(std::make_pair(var, phi));
    }
}

MarkerSRGBuilder::NodeT *MarkerSRGBuilder::readVariableRecursive(const DefSite& var, BlockT *block) {
    NodeT *val = nullptr;
    if (block->predecessorsNum() == 1) {
        val = readVariable(var, *block->predecessors().begin());
    } else {
        auto operandless_phi = std::unique_ptr<NodeT>(new NodeT(RDNodeType::PHI));
        val = operandless_phi.get();
        val->insertAfter(block->getFirstNode());
        block->prepend(val);

        writeVariable(var, val);
        phi_nodes.push_back(std::move(operandless_phi));

        auto phi = std::unique_ptr<NodeT>(new NodeT(RDNodeType::PHI));

        val = phi.get();
        val->insertAfter(block->getFirstNode());
        block->prepend(val);
        addPhiOperands(var, val);

        phi_nodes.push_back(std::move(phi));
    }
    writeVariable(var, val);
    return val;
}